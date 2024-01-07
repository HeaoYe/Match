#include "utils.hpp"
#include "camera.hpp"
#include <imgui.h>
#include <glm/gtx/rotate_vector.hpp>

struct PointLight {
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 color;
    alignas(4) float intensity;
};

struct GlobalInfo {
    alignas(16) glm::vec2 window_size;
    alignas(16) glm::vec3 clear_color;
    alignas(4) int scale;
};

class RayTracingApplication {
public:
    RayTracingApplication() {
        Match::setting.debug_mode = true;
        Match::setting.max_in_flight_frame = 1;
        Match::setting.window_size = { 1920, 1080 };
        Match::setting.default_font_filename = "/usr/share/fonts/TTF/JetBrainsMonoNerdFontMono-Light.ttf";
        Match::setting.chinese_font_filename = "/usr/share/fonts/adobe-source-han-sans/SourceHanSansCN-Medium.otf";
        Match::setting.font_size = 24.0f;
        Match::setting.enable_ray_tracing = true;
        Match::setting.device_extensions.push_back(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
        ctx = &Match::Initialize();
        Match::runtime_setting->set_vsync(false);
        Match::runtime_setting->set_multisample_count(Match::SampleCount::e1);

        scene_factory = ctx->create_resource_factory("../Scene/resource");
        factory = ctx->create_resource_factory("resource");

        auto builder = factory->create_render_pass_builder();
        builder->add_attachment("depth", Match::AttachmentType::eDepth)
            .add_subpass("main")
            .attach_depth_attachment("depth")
            .attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
        renderer = factory->create_renderer(builder);
        renderer->attach_render_layer<Match::ImGuiLayer>("imgui");

        vertex_buffer = factory->create_vertex_buffer(sizeof(Match::VertexBuffer), 4096000);
        index_buffer = factory->create_index_buffer(Match::IndexType::eUint32, 4096000);

        create_as();

        auto rg = factory->compile_shader("raytrace.rgen", Match::ShaderStage::eRaygen);
        auto miss = factory->compile_shader("raytrace.rmiss", Match::ShaderStage::eMiss);
        auto miss_shadow = factory->compile_shader("raytrace_shadow.rmiss", Match::ShaderStage::eMiss);
        auto closest = factory->compile_shader("raytrace.rchit", Match::ShaderStage::eClosestHit);

        ds = factory->create_descriptor_set(renderer);
        ds->add_descriptors({
            { Match::ShaderStage::eRaygen | Match::ShaderStage::eClosestHit, 0, Match::DescriptorType::eAccelerationStructure },
            { Match::ShaderStage::eRaygen, 1, Match::DescriptorType::eStorageImage },
            { Match::ShaderStage::eClosestHit, 2, Match::DescriptorType::eStorageBuffer },
        }).allocate();

        shared_ds = factory->create_descriptor_set(renderer);
        shared_ds->add_descriptors({
            { Match::ShaderStage::eFragment | Match::ShaderStage::eRaygen, 0, Match::DescriptorType::eUniform },
            { Match::ShaderStage::eVertex | Match::ShaderStage::eRaygen, 1, Match::DescriptorType::eUniform },
            { Match::ShaderStage::eFragment | Match::ShaderStage::eClosestHit, 2, Match::DescriptorType::eUniform },
        }).allocate();

        auto [width, height] = Match::runtime_setting->get_window_size();
        image = factory->create_storage_image(width, height, vk::Format::eR32G32B32A32Sfloat);
        ds->bind_storage_image(1, image);
        ds->bind_storage_buffer(2, instance->get_instance_addresses_bufer());
        callback_id = renderer->register_resource_recreate_callback([this]() {
            image.reset();
            auto [width, height] = Match::runtime_setting->get_window_size();
            MCH_INFO("Recreate {} {}", width, height)
            image = factory->create_storage_image(width, height, vk::Format::eR32G32B32A32Sfloat);
            ds->bind_storage_image(1, image);
            ds_ra->bind_texture(0, image, sampler);
            camera->update_aspect();
            camera->upload_data();
        });

        ds->bind_ray_tracing_instance(0, instance);

        rtp = factory->create_ray_tracing_shader_program();
        rtp->attach_raygen_shader(rg)
            .attach_miss_shader(miss)
            .attach_miss_shader(miss_shadow)
            .attach_closest_hit_shader(closest)
            .attach_descriptor_set(ds, 0)
            .attach_descriptor_set(shared_ds, 1)
            .compile({
                .max_ray_recursion_depth = 2
            });

        sp = factory->create_shader_program(renderer, "main");
        auto vs = factory->compile_shader("shader.vert", Match::ShaderStage::eVertex);
        auto fs = factory->compile_shader("shader.frag", Match::ShaderStage::eFragment);
        ds_ra = factory->create_descriptor_set(renderer);
        ds_ra->add_descriptors({  
            { Match::ShaderStage::eFragment, 0, Match::DescriptorType::eTexture },
        });
        sp->attach_vertex_shader(vs)
            .attach_fragment_shader(fs)
            .attach_descriptor_set(ds_ra)
            .attach_descriptor_set(shared_ds, 1)
            .compile({
                .cull_mode = Match::CullMode::eNone,
                .depth_test_enable = VK_FALSE,
                .dynamic_states = {
                    vk::DynamicState::eViewport,
                    vk::DynamicState::eScissor,
                }
            });
        sampler = factory->create_sampler({
            .min_filter = Match::SamplerFilter::eLinear,
        });
        ds_ra->bind_texture(0, image, sampler);
        g = factory->create_uniform_buffer(sizeof(GlobalInfo));
        light = factory->create_uniform_buffer(sizeof(PointLight));
        shared_ds->bind_uniform(0, g);
        shared_ds->bind_uniform(2, light);
        g_info = static_cast<GlobalInfo *>(g->get_uniform_ptr());
        light_ptr = static_cast<PointLight *>(light->get_uniform_ptr());
        light_ptr->pos = { 2, 2, 2 };
        light_ptr->color = { 1, 1, 1 };
        light_ptr->intensity = 1;
        camera = std::make_unique<Camera>(*factory);
        camera->data.pos = { 0, 0, -4 };
        camera->upload_data();
        shared_ds->bind_uniform(1, camera->uniform);

        auto vas = factory->create_vertex_attribute_set({
            Match::Vertex::generate_input_binding(0)
        });
        raster_sp = factory->create_shader_program(renderer, "main");
        raster_sp->attach_vertex_shader(factory->compile_shader("raster.vert", Match::ShaderStage::eVertex))
            .attach_fragment_shader(factory->compile_shader("raster.frag", Match::ShaderStage::eFragment))
            .attach_vertex_attribute_set(vas)
            .attach_descriptor_set(shared_ds)
            .compile({
                .cull_mode = Match::CullMode::eBack,
                .front_face = Match::FrontFace::eCounterClockwise,
                .depth_test_enable = VK_TRUE,
                .dynamic_states = {
                    vk::DynamicState::eViewport,
                    vk::DynamicState::eScissor,
                }
            });
    }

    ~RayTracingApplication() {
        renderer->wait_for_destroy();
        model.reset();
        model2.reset();
        instance.reset();
        camera.reset();
        renderer->remove_resource_recreate_callback(callback_id);
        image.reset();
        ds.reset();
        ds_ra.reset();
        shared_ds.reset();
        g.reset();
        light.reset();
        sampler.reset();
        sp.reset();
        raster_sp.reset();
        rtp.reset();
        renderer.reset();
        vertex_buffer.reset();
        index_buffer.reset();
        Match::Destroy();
    }

    void create_as() {
        // 加载模型
        model = scene_factory->load_model("mori_knob.obj");
        model2 = factory->load_model("cube.obj");
        // 创建builder
        auto builder = factory->create_acceleration_structure_builder();
        // 添加要构建加速结构的模型到builder的模型缓存
        builder->add_model(model);
        builder->add_model(model2);
        // 批量构建加速结构，以256MB为一批构建加速结构，此前添加的模型会完成加速结构的构建并清空builder的模型缓存
        builder->build();

        // 创建光线追踪Instance，也就是TopLevelAccelerationStructure，该实例包含两个模型
        // instance = factory->create_ray_tracing_instance({ model, model2 });
        // 只包含一个模型
        instance = factory->create_ray_tracing_instance({ model });

        // 上传模型顶点数据到缓存，光栅化渲染用
        model2->upload_data(vertex_buffer, index_buffer, model->upload_data(vertex_buffer, index_buffer));
    }

    void gameloop() {
        while (Match::window->is_alive()) {
            Match::window->poll_events();
            camera->update(ImGui::GetIO().DeltaTime);
            static float time = 0;
            time += ImGui::GetIO().DeltaTime;
            light_ptr->pos = glm::rotateY(glm::vec3(2, light_ptr->pos.y, 2), time);
            renderer->set_clear_value(Match::SWAPCHAIN_IMAGE_ATTACHMENT, { { g_info->clear_color.r, g_info->clear_color.g, g_info->clear_color.b, 1.0f } });

            renderer->acquire_next_image();

            if (is_ray_tracing) {
                auto cmd_buf = renderer->get_command_buffer();

                auto [width, height] = Match::runtime_setting->get_window_size();
                g_info->window_size = { static_cast<float>(width), static_cast<float>(height) };
                
                renderer->bind_shader_program(rtp);
                renderer->trace_rays(width, height, 1);

                renderer->begin_render_pass();

                renderer->bind_shader_program(sp);
                renderer->set_scissor(0, 0, width, height);
                renderer->set_viewport(0, height, width, -static_cast<float>(height));
                renderer->draw(3, 2, 0, 0);
            } else {
                renderer->begin_render_pass();

                auto [width, height] = Match::runtime_setting->get_window_size();
                renderer->bind_shader_program(raster_sp);
                renderer->set_scissor(0, 0, width, height);
                renderer->set_viewport(0, height, width, -static_cast<float>(height));
                renderer->bind_vertex_buffer(vertex_buffer);
                renderer->bind_index_buffer(index_buffer);
                renderer->draw_model(model, 1, 0);
                // renderer->draw_model(model2, 1, 0);
            }

            renderer->begin_layer_render("imgui");
            ImGui::Text("FrameRate: %f", ImGui::GetIO().Framerate);
            ImGui::Checkbox("Enabel Ray Tracing", &is_ray_tracing);
            ImGui::SliderInt("Ray Tracing Scale", &g_info->scale, 1, 5);
            ImGui::SliderFloat3("Clear Color", &g_info->clear_color.r, 0, 1);
            ImGui::SliderFloat3("Ligjt Color", &light_ptr->color.r, 0, 1);
            ImGui::SliderFloat("Ligjt Intensity", &light_ptr->intensity, 0, 10);
            ImGui::SliderFloat("Ligjt Height", &light_ptr->pos.y, -5, 5);
            renderer->end_layer_render("imgui");

            renderer->end_render_pass();

            renderer->present();
        }
    }
private:
    std::shared_ptr<Match::Model> model;
    std::shared_ptr<Match::Model> model2;
    std::shared_ptr<Match::RayTracingInstance> instance;
    std::shared_ptr<Match::RayTracingShaderProgram> rtp;

    std::shared_ptr<Match::ResourceFactory> scene_factory;
    std::shared_ptr<Match::ResourceFactory> factory;
    std::shared_ptr<Match::VertexBuffer> vertex_buffer;
    std::shared_ptr<Match::IndexBuffer> index_buffer;
    std::shared_ptr<Match::Renderer> renderer;
    std::shared_ptr<Match::DescriptorSet> ds;
    std::shared_ptr<Match::Sampler> sampler;
    std::shared_ptr<Match::GraphicsShaderProgram> sp;
    std::shared_ptr<Match::DescriptorSet> ds_ra;
    std::shared_ptr<Match::DescriptorSet> shared_ds;
    std::shared_ptr<Match::StorageImage> image;
    std::shared_ptr<Match::UniformBuffer> g;
    GlobalInfo *g_info;
    std::shared_ptr<Match::UniformBuffer> light;
    PointLight *light_ptr;
    std::unique_ptr<Camera> camera;
    uint32_t callback_id;

    bool is_ray_tracing = false;
    std::shared_ptr<Match::GraphicsShaderProgram> raster_sp;
};

int main() {
    RayTracingApplication app {};
    // 刚做完底层加速结构和顶层加速结构的创建。。。。
    // 一个简单的光追场景，包括一个模型，一个点光源，以及阴影
    // 未来会集成到渲染引擎中
    app.gameloop();
}
