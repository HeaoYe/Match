#include "ray_tracing_v2_scene.hpp"
#include <random>

void RayTracingV2Scene::initialize() {
    this->is_ray_tracing_scene = true;
    auto render_pass_builder = factory->create_render_pass_builder();
    render_pass_builder->add_subpass("main")
        .attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
    renderer = factory->create_renderer(render_pass_builder);
    renderer->attach_render_layer<Match::ImGuiLayer>("imgui layer");

    camera = std::make_unique<Camera>(*factory);
    camera->data.pos = { 0, 0, -3 };
    camera->upload_data();

    sphere_collect = factory->create_sphere_collect();
    sphere_collect->register_custom_sphere_data<SphereMaterial>();

    std::random_device device;
    std::mt19937 generator(device());
    std::normal_distribution<float> distribution(0, 1);
    std::normal_distribution<float> scale_distribution(0.2, 0.1);
    std::uniform_real_distribution<float> color_distribution(0, 1);

    for (uint32_t i = 0; i < 100; i ++) {
        float r = scale_distribution(generator);
        glm::vec3 color {
            color_distribution(generator),
            color_distribution(generator),
            color_distribution(generator)
        };
        sphere_collect->add_sphere(
            0, 
            { distribution(generator) * 5, r, distribution(generator) * 5 }, 
            r,
            SphereMaterial {
                .albedo = color,
                .roughness = color_distribution(generator) * color_distribution(generator),
                .light_color = color,
                .light_intensity = color_distribution(generator) * color_distribution(generator),
            }
        );
    }

    sphere_collect->add_sphere(0, { -2, 1, 0 }, 0.4, SphereMaterial { .albedo = { 1, 0.4, 0.4 }, .roughness = 1 });
    sphere_collect->add_sphere(0, { 0, 1, 0 }, 0.7, SphereMaterial { .albedo = { 0.4, 1, 0.4 }, .roughness = 0.5 });
    sphere_collect->add_sphere(0, { 2, 1, 0 }, 1, SphereMaterial { .albedo = { 0.4, 0.4, 1 } });
    sphere_collect->add_sphere(0, {0, -500, 0 }, 500, SphereMaterial { .albedo = { 0.8, 0.5, 0.25 }, .roughness = 0, .light_intensity = 0 });

    auto builder = factory->create_acceleration_structure_builder();
    builder->add_model(sphere_collect);
    builder->build();
    builder.reset();

    instance_collect = factory->create_ray_tracing_instance_collect();
    instance_collect->add_instance(0, sphere_collect, glm::mat4(1), 0);
    instance_collect->build();

    auto [width, height] = Match::runtime_setting->get_window_size();
    ray_tracing_result_image = factory->create_storage_image(width, height, vk::Format::eR32G32B32A32Sfloat);
    renderer->register_resource_recreate_callback([&]() {
        ray_tracing_result_image.reset();
        auto [width, height] = Match::runtime_setting->get_window_size();
        ray_tracing_result_image = factory->create_storage_image(width, height, vk::Format::eR32G32B32A32Sfloat);
        ray_tracing_shader_program_ds->bind_storage_image(0, ray_tracing_result_image);
        shader_program_ds->bind_texture(0, ray_tracing_result_image, sampelr);
        camera->update_aspect();
        camera->upload_data();
    });

    auto raygen_shader = factory->compile_shader("ray_tracing_v2_shader/rt.rgen", Match::ShaderStage::eRaygen);
    auto miss_shader = factory->compile_shader("ray_tracing_v2_shader/rt.rmiss", Match::ShaderStage::eMiss);
    auto closest_hit_shader = factory->compile_shader("ray_tracing_v2_shader/rt.rchit", Match::ShaderStage::eClosestHit);
    auto intersection_shader = factory->compile_shader("ray_tracing_v2_shader/rt.rint", Match::ShaderStage::eIntersection);

    ray_tracing_shader_program_constants = factory->create_push_constants(
        Match::ShaderStage::eRaygen, 
        {
            { "time", Match::ConstantType::eFloat },
            { "sample_count", Match::ConstantType::eInt32 },
            { "frame_count", Match::ConstantType::eInt32 },
        }
    );

    ray_tracing_shader_program_ds = factory->create_descriptor_set(renderer);
    ray_tracing_shader_program_ds->add_descriptors({
        { Match::ShaderStage::eRaygen, 0, Match::DescriptorType::eStorageImage },
        { Match::ShaderStage::eRaygen | Match::ShaderStage::eClosestHit, 1, Match::DescriptorType::eRayTracingInstance },
        { Match::ShaderStage::eRaygen, 2, Match::DescriptorType::eUniform },
        { Match::ShaderStage::eIntersection | Match::ShaderStage::eClosestHit, 3, Match::DescriptorType::eStorageBuffer },
        { Match::ShaderStage::eClosestHit, 4, Match::DescriptorType::eStorageBuffer },
    })
        .allocate()
        .bind_storage_image(0, ray_tracing_result_image)
        .bind_ray_tracing_instance_collect(1, instance_collect)
        .bind_uniform(2, camera->uniform)
        .bind_storage_buffer(3, sphere_collect->get_spheres_buffer())
        .bind_storage_buffer(4, sphere_collect->get_custom_data_buffer<SphereMaterial>());
    
    ray_tracing_shader_program = factory->create_ray_tracing_shader_program();
    ray_tracing_shader_program->attach_raygen_shader(raygen_shader)
        .attach_miss_shader(miss_shader)
        .attach_hit_group(closest_hit_shader, intersection_shader)
        .attach_descriptor_set(ray_tracing_shader_program_ds)
        .attach_push_constants(ray_tracing_shader_program_constants)
        .compile({
            .max_ray_recursion_depth = 16,
        });

    auto vert_shader = factory->compile_shader("ray_tracing_v2_shader/shader.vert", Match::ShaderStage::eVertex);
    auto frag_shader = factory->compile_shader("ray_tracing_v2_shader/shader.frag", Match::ShaderStage::eFragment);

    sampelr = factory->create_sampler();

    shader_program_ds = factory->create_descriptor_set(renderer);
    shader_program_ds->add_descriptors({
        { Match::ShaderStage::eFragment, 0, Match::DescriptorType::eTexture },
    })
        .allocate()
        .bind_texture(0, ray_tracing_result_image, sampelr);

    shader_program = factory->create_shader_program(renderer, "main");
    shader_program->attach_vertex_shader(vert_shader)
        .attach_fragment_shader(frag_shader)
        .attach_descriptor_set(shader_program_ds)
        .compile({
            .cull_mode = Match::CullMode::eNone,
            .depth_test_enable = VK_FALSE,
            .dynamic_states = {
                vk::DynamicState::eViewport,
                vk::DynamicState::eScissor,
            }
        });
}

void RayTracingV2Scene::update(float dt) {
    camera->update(dt);

    static float time = 0;
    time += dt;
    ray_tracing_shader_program_constants->push_constant("time", time);

    static int frame_count = 0;
    if (camera->is_capture_cursor) {
        frame_count = 0;
    }
    ray_tracing_shader_program_constants->push_constant("frame_count", frame_count);
    frame_count ++;
}

void RayTracingV2Scene::render() {
    renderer->bind_shader_program(ray_tracing_shader_program);
    renderer->trace_rays();

    renderer->begin_render_pass();
    renderer->bind_shader_program(shader_program);
    auto [width, height] = Match::runtime_setting->get_window_size();
    renderer->set_viewport(0, static_cast<float>(height), static_cast<float>(width), -static_cast<float>(height));
    renderer->set_scissor(0, 0, width, height);
    renderer->draw(3, 1, 0, 0);
}

void RayTracingV2Scene::render_imgui() {
    ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);

    static int sample_count = 1;
    ImGui::SliderInt("Sample Count", &sample_count, 1, 16);
    ray_tracing_shader_program_constants->push_constant("sample_count", sample_count);
}

void RayTracingV2Scene::destroy() {
}
