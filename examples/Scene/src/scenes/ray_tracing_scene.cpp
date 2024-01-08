#include "ray_tracing_scene.hpp"
#include <glm/gtx/rotate_vector.hpp>

void RayTracingScene::initialize() {
    this->is_ray_tracing_scene = true;

    auto render_pass_builder = factory->create_render_pass_builder();
    render_pass_builder->add_subpass("main")
        .attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
    renderer = factory->create_renderer(render_pass_builder);
    renderer->attach_render_layer<Match::ImGuiLayer>("imgui layer");

    camera = std::make_unique<Camera>(*factory);
    camera->data.pos = glm::vec3(0, 0, -3);
    camera->upload_data();

    // 加载模型
    model1 = factory->load_model("mori_knob.obj");
    model2 = factory->load_model("dragon.obj");
    // 加载完的模型只支持光栅化渲染,需要构建光追加速结构后才可用于光追渲染
    auto builder = factory->create_acceleration_structure_builder();
    // 添加需要构建光追加速结构模型,可添加多个
    builder->add_model(model1);
    builder->add_model(model2);
    // 构建所有已添加模型的光追加速结构
    builder->build();
    // 之后模型就可以用于光追渲染了,builder也可以销毁了
    builder.reset();

    // 使用光追实例管理渲染的模型
    instance = factory->create_ray_tracing_instance({
        model1,
        model2,
    });

    // 简易光追资源初始化
    auto [width, height] = Match::runtime_setting->get_window_size();
    // 创建一个和窗口一样大的缓存图片,格式为精度更高的RGBA32而不是RGBA8
    ray_tracing_result_image = factory->create_storage_image(width, height, vk::Format::eR32G32B32A32Sfloat);

    // ray_tracing_shader_program 光追渲染程序
    // 包含一个光线发射器   多个未命中着色器   多个最近命中着色器
    // 光线由光线发射器发出,若未命中任何物体,调用未命中着色器,若命中,距离光线发出点最近的命中点调用最近命中着色器
    auto raygen_shader = factory->compile_shader("ray_tracing_shader/rt.rgen", Match::ShaderStage::eRaygen);
    auto miss_shader = factory->compile_shader("ray_tracing_shader/rt.rmiss", Match::ShaderStage::eMiss);
    auto miss_shodow_shader = factory->compile_shader("ray_tracing_shader/rt_shadow.rmiss", Match::ShaderStage::eMiss);
    auto closest_hit_shader = factory->compile_shader("ray_tracing_shader/rt.rchit", Match::ShaderStage::eClosestHit);

    ray_tracing_shader_program_ds = factory->create_descriptor_set(renderer);
    ray_tracing_shader_program_ds->add_descriptors({
        { Match::ShaderStage::eRaygen, 0, Match::DescriptorType::eStorageImage },
        { Match::ShaderStage::eRaygen | Match::ShaderStage::eClosestHit, 1, Match::DescriptorType::eRayTracingInstance },
        { Match::ShaderStage::eRaygen, 2, Match::DescriptorType::eUniform },
        // 模型的VertexBuffer和IndexBuffer的地址信息
        { Match::ShaderStage::eClosestHit, 3, Match::DescriptorType::eStorageBuffer },
    });

    ray_tracing_shader_program_constants = factory->create_push_constants(
        Match::ShaderStage::eClosestHit | Match::ShaderStage::eMiss | Match::ShaderStage::eRaygen,
        {
            // 天空颜色
            { "sky_color", Match::ConstantType::eFloat3 },
            // 点光源位置
            { "light_pos", Match::ConstantType::eFloat3 },
            // 点光源颜色
            { "light_color", Match::ConstantType::eFloat3 },
            // 点光源强度
            { "light_intensity", Match::ConstantType::eFloat },
            // 采样点
            { "sample_count", Match::ConstantType::eInt32 },
        }
    );

    // 未命中着色器被attach到ray_tracing_shader_program的顺序决定了它的索引
    ray_tracing_shader_program = factory->create_ray_tracing_shader_program();
    ray_tracing_shader_program->attach_raygen_shader(raygen_shader)
        .attach_miss_shader(miss_shader)            // 索引为0
        .attach_miss_shader(miss_shodow_shader)     // 索引为1
        .attach_closest_hit_shader(closest_hit_shader)
        .attach_descriptor_set(ray_tracing_shader_program_ds)
        .attach_push_constants(ray_tracing_shader_program_constants)
        .compile({
            .max_ray_recursion_depth = 2,  // 配置最大递归深度,因为在最近命中着色器中调用了traceRayEXT,所以最大递归深度为2,
        });
    ray_tracing_shader_program_ds->bind_storage_image(0, ray_tracing_result_image);
    // 将需要渲染的模型传入光线发射器
    ray_tracing_shader_program_ds->bind_ray_tracing_instance(1, instance);
    ray_tracing_shader_program_ds->bind_uniform(2, camera->uniform);
    ray_tracing_shader_program_ds->bind_storage_buffer(3, instance->get_instance_addresses_bufer());

    // 基础光栅化资源初始化
    auto vert_shader = factory->compile_shader("ray_tracing_shader/shader.vert", Match::ShaderStage::eVertex);
    auto frag_shader = factory->compile_shader("ray_tracing_shader/shader.frag", Match::ShaderStage::eFragment);
    sampler = factory->create_sampler();

    shader_program_ds = factory->create_descriptor_set(renderer);
    shader_program_ds->add_descriptors({
        // ray_tracing_result_image
        { Match::ShaderStage::eFragment, 0, Match::DescriptorType::eTexture }
    });

    shader_program = factory->create_shader_program(renderer, "main");
    shader_program->attach_vertex_shader(vert_shader)
        .attach_fragment_shader(frag_shader)
        .attach_descriptor_set(shader_program_ds)
        .compile({
            .cull_mode = Match::CullMode::eNone,
            .depth_test_enable = VK_FALSE,
        });
    shader_program_ds->bind_texture(0, ray_tracing_result_image, sampler);
}

void RayTracingScene::update(float dt) {
    camera->update(dt);

    static float time = 0;
    time += dt;
    glm::vec3 light_pos = glm::rotateY(glm::vec3(2, 2, 2), time);
    ray_tracing_shader_program_constants->push_constant("light_pos", &light_pos);
}

void RayTracingScene::render() {
    // 光追渲染
    // 光追类似以计算,并不是直接将画面输出到屏幕上,而是将光追的结果储存在缓存图片中,再由光栅化程序渲染到屏幕
    // 因为光追是计算,不应该在RenderPass中进行,而Scene::render()在调用前已经开启了RenderPass,所以会报错
    renderer->bind_shader_program(ray_tracing_shader_program);
    // 进行光追计算,也就是调用光线发射器,缓存图片的(宽高深)是(Width*Height*Depth)  (1920 * 1080 * 1),所以为每个像素都调用一次光线发射器,
    // renderer->trace_rays() 默认 Width为窗口宽, Height为窗口高, Depth为1
    // 之后会调用光线发射器 Width*Height*Depth 次,每次调用都会分配一个gl_LaunchIDEXT,代表W,H,D三个维度的索引,gl_LaunchSizeEXT代表总的W,H,D
    auto [width, height] = Match::runtime_setting->get_window_size();
    renderer->trace_rays(width, height, 1);

    // 基础光栅化渲染,将ray_tracing_result_image渲染到屏幕
    renderer->begin_render_pass();
    renderer->bind_shader_program(shader_program);
    renderer->draw(3, 2, 0, 0);
}

void RayTracingScene::render_imgui() {
    ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);

    static glm::vec3 sky_color { 0.8, 0.8, 0.9 };
    ImGui::SliderFloat3("Sky Color", &sky_color.r, 0, 1);
    ray_tracing_shader_program_constants->push_constant("sky_color", &sky_color);

    static glm::vec3 light_color { 0.5, 0.8, 0.7 };
    ImGui::SliderFloat3("Light Color", &light_color.r, 0, 1);
    ray_tracing_shader_program_constants->push_constant("light_color", &light_color);

    static float light_intensity = 4;
    ImGui::SliderFloat("Light Intensity", &light_intensity, 1, 10);
    ray_tracing_shader_program_constants->push_constant("light_intensity", light_intensity);

    static int sample_count = 1;
    ImGui::SliderInt("Sample Count", &sample_count, 1, 10);
    ray_tracing_shader_program_constants->push_constant("sample_count", sample_count);
}

void RayTracingScene::destroy() {
}
