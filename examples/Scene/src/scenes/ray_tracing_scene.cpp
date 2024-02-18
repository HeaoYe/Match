#include "ray_tracing_scene.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <random>

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

    std::random_device device;
    std::mt19937 generator(device());
    std::normal_distribution<float> distribution(0, 1);
    std::normal_distribution<float> scale_distribution(0.3, 0.2);
    std::uniform_real_distribution<float> color_distribution(0, 1);

    // 加载模型
    model1 = factory->load_model("mori_knob.obj");
    model2 = factory->load_model("dragon.obj");
    // sphere_collect的管理方式与instance_collect类似,添加球体时要指定球体的group_id,更新时以group为单位更新
    sphere_collect = factory->create_sphere_collect();
    // 注册自定义数据
    sphere_collect->register_custom_sphere_data<MyCustomData1>();
    // 可以添加多个球体
    for (uint32_t i = 0; i < 300; i ++) {
        glm::vec3 center {
            distribution(generator) * 2,
            distribution(generator) * 2,
            distribution(generator) * 2,
        };
        float radius = scale_distribution(generator) / 4;
        auto rotate_vector = glm::vec3(distribution(generator), distribution(generator), distribution(generator));
        sphere_datas.push_back(std::make_pair(Match::SphereData { center, radius }, rotate_vector));
        sphere_collect->add_sphere(0, center, radius, MyCustomData1 { .color = { color_distribution(generator), color_distribution(generator), color_distribution(generator) } });
    }

    // 加载完的模型只支持光栅化渲染,需要构建光追加速结构后才可用于光追渲染
    auto builder = factory->create_acceleration_structure_builder();
    // 添加需要构建光追加速结构模型,可添加多个
    builder->add_model(model1);
    builder->add_model(model2);
    // 构建所有已添加模型的光追加速结构
    // 构建时指定光追加速结构是否可以更新
    builder->build(false);
    // 构建可更新的球体集合光追加速结构
    builder->add_model(sphere_collect);
    builder->build(true);
    // 之后模型就可以用于光追渲染了,builder也可以销毁了
    builder.reset();

    // 使用光追实例管理渲染的模型
    // 不再在创建光追实例时传入模型信息
    // 创建实例集合
    instance_collect = factory->create_ray_tracing_instance_collect();
    // 为实例集合注册自定义数据
    // 可以注册多个
    instance_collect->register_custom_instance_data<MyCustomData1>();
    instance_collect->register_custom_instance_data<MyCustomData2>();
    // 使用add_instance添加模型,添加时要指定添加到的组,以及模型变换到世界空间的矩阵
    // 使用add_instance添加模型,在参数列表的最后添加自定义数据,不指定的话会调用默认构造添加自定义数据
    // 使用add_instance添加模型,需要指定命中组的索引
    instance_collect->add_instance(0, model1, glm::mat4(1), 0, MyCustomData2 { .color_scale = 0.3f });  // index = 0
    instance_collect->add_instance(2, sphere_collect, glm::mat4(1), 1);
    // 实例化渲染500条中国龙
    // 很明显的渲染错误
    for (uint32_t index = 0; index < 500; index ++) {
        auto pos = glm::vec3(distribution(generator) * 2, distribution(generator) * 2, distribution(generator) * 2);
        auto rotate_vector = glm::vec3(distribution(generator), distribution(generator), distribution(generator));
        auto start_angle = distribution(generator) * 3.1415926535f;
        auto scale = scale_distribution(generator);
        instance_transform_infos.push_back({
            pos, rotate_vector, start_angle, scale,
        });
        auto transform_mat = glm::translate(pos) * glm::rotate(start_angle, rotate_vector) * glm::scale(glm::mat4(1), glm::vec3(scale));
        auto my_data = MyCustomData1 {
            .color = {
                color_distribution(generator),
                color_distribution(generator),
                color_distribution(generator),
            }
        };
        instance_collect->add_instance(1, model2, transform_mat, 0, my_data, MyCustomData2 { .color_scale = 0.5f });
    }
    // 使用build构建光追实例的加速结构
    instance_collect->build();
    // build之后无法继续添加模型,但可以使用update更新实例信息
    // instance_collect->update(...);

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
    auto sphere_closest_hit_shader = factory->compile_shader("ray_tracing_shader/rt_sphere.rchit", Match::ShaderStage::eClosestHit);
    auto sphere_intersection_shader = factory->compile_shader("ray_tracing_shader/rt_sphere.rint", Match::ShaderStage::eIntersection);

    ray_tracing_shader_program_ds = factory->create_descriptor_set(renderer);
    ray_tracing_shader_program_ds->add_descriptors({
        { Match::ShaderStage::eRaygen, 0, Match::DescriptorType::eStorageImage },
        { Match::ShaderStage::eRaygen | Match::ShaderStage::eClosestHit, 1, Match::DescriptorType::eRayTracingInstance },
        { Match::ShaderStage::eRaygen, 2, Match::DescriptorType::eUniform },
        // 模型的VertexBuffer和IndexBuffer的地址信息
        { Match::ShaderStage::eClosestHit, 3, Match::DescriptorType::eStorageBuffer },
        // 模型的自定义数据信息,每种自定义数据都需要用单独的一个StorageBuffer传入着色器
        { Match::ShaderStage::eClosestHit, 4, Match::DescriptorType::eStorageBuffer },    // MyCustomData1
        { Match::ShaderStage::eClosestHit, 5, Match::DescriptorType::eStorageBuffer },    // MyCustomData2
        { Match::ShaderStage::eIntersection, 6, Match::DescriptorType::eStorageBuffer },  // 球体信息
        { Match::ShaderStage::eClosestHit, 7, Match::DescriptorType::eStorageBuffer },    // 球体自定义信息
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
        // 添加命中组,索引为0
        .attach_hit_group(closest_hit_shader)
        // 添加命中组,索引为1
        .attach_hit_group(sphere_closest_hit_shader, sphere_intersection_shader)
        .attach_descriptor_set(ray_tracing_shader_program_ds)
        .attach_push_constants(ray_tracing_shader_program_constants)
        .compile({
            .max_ray_recursion_depth = 2,  // 配置最大递归深度,因为在最近命中着色器中调用了traceRayEXT,所以最大递归深度为2,
        });
    ray_tracing_shader_program_ds->bind_storage_image(0, ray_tracing_result_image);
    // 将需要渲染的模型传入光线发射器
    ray_tracing_shader_program_ds->bind_ray_tracing_instance_collect(1, instance_collect);
    ray_tracing_shader_program_ds->bind_uniform(2, camera->uniform);
    // 默认实例的数据是该实例对应模型的VertexBuffer和IndexBuffer的地址
    ray_tracing_shader_program_ds->bind_storage_buffer(3, instance_collect->get_instance_address_data_buffer());
    // 绑定自定义数据的缓存
    ray_tracing_shader_program_ds->bind_storage_buffer(4, instance_collect->get_custom_instance_data_buffer<MyCustomData1>());
    ray_tracing_shader_program_ds->bind_storage_buffer(5, instance_collect->get_custom_instance_data_buffer<MyCustomData2>());
    ray_tracing_shader_program_ds->bind_storage_buffer(6, sphere_collect->get_spheres_buffer());
    ray_tracing_shader_program_ds->bind_storage_buffer(7, sphere_collect->get_custom_data_buffer<MyCustomData1>());

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

    // 以组为单位更新
    // 更新自定义实例数据
    // 借用了ecs的思想,每个实例是 实体(E) 每种自定义实例数据都是组件(C) instance_collect是更新所有实体的某一组件的系统(S)
    // 每种自定义实例数据都有自己的缓存,更新时以组为单位,更新该组某个类型的自定义实例数据
    instance_collect->update<MyCustomData1>(0, [&](uint32_t index, MyCustomData1 &my_data) {
        my_data.color = {
            (std::cos(time + 2 * 3.1415926535f * 1 / 3) + 1) / 2,
            (std::cos(time + 2 * 3.1415926535f * 2 / 3) + 1) / 2,
            (std::cos(time + 2 * 3.1415926535f * 3 / 3) + 1) / 2,
        };
    });
    instance_collect->update<MyCustomData2>(1, [&](uint32_t index, auto &my_data) {
        my_data.color_scale = (std::cos(time) + 1) / 2;
    });
    // 更新sphere_collect的底层光追加速结构
    sphere_collect->update(0, [&](uint32_t index, auto &sphere) {
        auto [raw_sphere, rotate_vector] = sphere_datas[index];
        auto [pos, scale] = raw_sphere;
        auto distance3 = glm::pow(glm::length(pos), 3.0f);
        auto k = 2.0f / distance3;
        // 计算角速度
        auto w = glm::sqrt(k);
        sphere.center = glm::rotate(w * time * 1.2f, rotate_vector) * glm::vec4(pos.x, pos.y, pos.z, 1);
        sphere.radius = (cos(time * 1.8) + 2) / 4 * scale;
    });
    // 因为更新了底层光追加速结构,所以要更新instance_collect中sphere_collect的光追加速结构
    instance_collect->update(2, [&](uint32_t index, auto set_transform, auto update_acceleration_structure) {
        update_acceleration_structure(sphere_collect);
    });
    instance_collect->update(1, [&](uint32_t index, auto set_transform, auto) {
        auto [pos, rotate_vector, start_angle, scale] = instance_transform_infos.at(index);
        start_angle += time;
        scale *= (cos(time * 1.8 + start_angle) + 3) / 3;
        // 在天体运动中, 一个天体绕中心天体运动的角速度的平方与该天体到中心天体距离的立方成反比
        auto distance3 = glm::pow(glm::length(pos), 3.0f);
        auto k = 2.0f / distance3;
        // 计算角速度
        auto w = glm::sqrt(k);
        pos = glm::rotate(w * time, rotate_vector) * glm::vec4(pos.x, pos.y, pos.z, 1);
        auto transform_mat = glm::translate(pos) * glm::rotate(start_angle, rotate_vector) * glm::scale(glm::mat4(1), glm::vec3(scale));
        set_transform(transform_mat);
    });
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
