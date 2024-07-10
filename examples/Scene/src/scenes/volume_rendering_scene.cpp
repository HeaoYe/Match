#include "volume_rendering_scene.hpp"

void VolumeRenderingScene::initialize() {
    // 设置日志级别
    Match::set_log_level(Match::LogLevel::eDebug);

    this->is_ray_tracing_scene = true;
    auto render_pass_builder = factory->create_render_pass_builder();
    render_pass_builder->add_subpass("main")
        .attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
    renderer = factory->create_renderer(render_pass_builder);
    renderer->attach_render_layer<Match::ImGuiLayer>("imgui layer");

    args = factory->create_uniform_buffer(sizeof(VolumeRenderingArgs));
    auto *ptr = static_cast<VolumeRenderingArgs *>(args->get_uniform_ptr());
    *ptr = VolumeRenderingArgs {};

    volume_data_cloud = factory->load_volume_data("wdas_cloud_density.match_volume_data");
    volume_buffer_cloud = factory->create_storage_buffer(sizeof(float) * Match::volume_raw_data_resolution * Match::volume_raw_data_resolution * Match::volume_raw_data_resolution);
    volume_data_cloud->upload_to_buffer(volume_buffer_cloud);

    volume_data_smoke = factory->load_volume_data("bunny_cloud_density.match_volume_data");
    volume_buffer_smoke = factory->create_storage_buffer(sizeof(float) * Match::volume_raw_data_resolution * Match::volume_raw_data_resolution * Match::volume_raw_data_resolution);
    volume_data_smoke->upload_to_buffer(volume_buffer_smoke);

    camera = std::make_unique<Camera>(*factory);
    camera->data.pos = { 0, 3, -11.5 };
    camera->upload_data();

    sphere_collect = factory->create_sphere_collect();
    sphere_collect->register_custom_sphere_data<SphereType>();

    sphere_collect->add_sphere(0, {0, -500, 0 }, 500, SphereType::eGround);
    sphere_collect->add_sphere(0, {3.5, 3.5, 4 }, 3, SphereType::eUniform);
    sphere_collect->add_sphere(0, {-3.5, 3.5, 4 }, 3, SphereType::ePerlin);
    sphere_collect->add_sphere(0, {5.5, 5.5, -6 }, 5, SphereType::eCloud);
    sphere_collect->add_sphere(0, {-5.5, 5.5, -6 }, 5, SphereType::eSmoke);

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

    auto raygen_shader = factory->compile_shader("volume_rendering_shader/rt.rgen", Match::ShaderStage::eRaygen);
    auto miss_shader = factory->compile_shader("volume_rendering_shader/rt.rmiss", Match::ShaderStage::eMiss);
    auto closest_hit_shader = factory->compile_shader("volume_rendering_shader/sphere.rchit", Match::ShaderStage::eClosestHit);
    auto intersection_shader = factory->compile_shader("volume_rendering_shader/sphere.rint", Match::ShaderStage::eIntersection);

    ray_tracing_shader_program_constants = factory->create_push_constants(
        Match::ShaderStage::eRaygen,
        {
            { "time", Match::ConstantType::eFloat },
            { "sample_count", Match::ConstantType::eInt32 },
            { "frame_count", Match::ConstantType::eInt32 },
            { "max_ray_recursion_depth", Match::ConstantType::eInt32 },
        }
    );

    ray_tracing_shader_program_ds = factory->create_descriptor_set(renderer);
    ray_tracing_shader_program_ds->add_descriptors({
        { Match::ShaderStage::eRaygen, 0, Match::DescriptorType::eStorageImage },
        { Match::ShaderStage::eRaygen | Match::ShaderStage::eClosestHit, 1, Match::DescriptorType::eRayTracingInstance },
        { Match::ShaderStage::eRaygen, 2, Match::DescriptorType::eUniform },
        { Match::ShaderStage::eIntersection | Match::ShaderStage::eClosestHit, 3, Match::DescriptorType::eStorageBuffer },
        { Match::ShaderStage::eClosestHit, 4, Match::DescriptorType::eStorageBuffer },
        { Match::ShaderStage::eClosestHit, 5, Match::DescriptorType::eStorageBuffer },
        { Match::ShaderStage::eClosestHit, 6, Match::DescriptorType::eUniform },
        { Match::ShaderStage::eClosestHit, 7, Match::DescriptorType::eStorageBuffer },
        { Match::ShaderStage::eClosestHit, 8, Match::DescriptorType::eStorageBuffer },
    })
        .allocate()
        .bind_storage_image(0, ray_tracing_result_image)
        .bind_ray_tracing_instance_collect(1, instance_collect)
        .bind_uniform(2, camera->uniform)
        .bind_storage_buffer(3, sphere_collect->get_spheres_buffer())
        .bind_storage_buffer(4, instance_collect->get_instance_address_data_buffer())
        .bind_storage_buffer(5, sphere_collect->get_custom_data_buffer<SphereType>())
        .bind_uniform(6, args)
        .bind_storage_buffer(7, volume_buffer_cloud)
        .bind_storage_buffer(8, volume_buffer_smoke);

    ray_tracing_shader_program = factory->create_ray_tracing_shader_program();
    ray_tracing_shader_program->attach_raygen_shader(raygen_shader)
        .attach_miss_shader(miss_shader)
        .attach_hit_group(closest_hit_shader, intersection_shader)
        .attach_descriptor_set(ray_tracing_shader_program_ds)
        .attach_push_constants(ray_tracing_shader_program_constants)
        .compile({
            .max_ray_recursion_depth = 1,  // 因为提取了光线递归,所以最大深度为1,这样可以加速光追渲染
        });

    auto vert_shader = factory->compile_shader("volume_rendering_shader/shader.vert", Match::ShaderStage::eVertex);
    auto frag_shader = factory->compile_shader("volume_rendering_shader/shader.frag", Match::ShaderStage::eFragment);

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

void VolumeRenderingScene::update(float dt) {
    camera->update(dt);

    static float time = 0;
    time += dt;
    ray_tracing_shader_program_constants->push_constant("time", time);

    if (camera->is_capture_cursor) {
        frame_count = 0;
    }
    ray_tracing_shader_program_constants->push_constant("frame_count", frame_count);
    frame_count ++;
}

void VolumeRenderingScene::render() {
    renderer->bind_shader_program(ray_tracing_shader_program);
    renderer->trace_rays();

    renderer->begin_render_pass();
    renderer->bind_shader_program(shader_program);
    auto [width, height] = Match::runtime_setting->get_window_size();
    renderer->set_viewport(0, static_cast<float>(height), static_cast<float>(width), -static_cast<float>(height));
    renderer->set_scissor(0, 0, width, height);
    renderer->draw(3, 1, 0, 0);
}

void VolumeRenderingScene::render_imgui() {
    ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);

    bool changed = false;

    static int sample_count = 1;
    changed |= ImGui::SliderInt("Sample Count", &sample_count, 1, 16);
    ray_tracing_shader_program_constants->push_constant("sample_count", sample_count);

    static int max_ray_recursion_depth = 6;
    changed |= ImGui::SliderInt("Max Ray Recursion Depth", &max_ray_recursion_depth, 1, 16);
    ray_tracing_shader_program_constants->push_constant("max_ray_recursion_depth", max_ray_recursion_depth);

    auto *ptr = static_cast<VolumeRenderingArgs *>(args->get_uniform_ptr());
    ImGui::SeparatorText("Uniform");
    changed |= ImGui::SliderFloat("U Sigma Maj", &ptr->sigma_maj_uniform, 0, 10);
    changed |= ImGui::SliderFloat("U Sigma A", &ptr->sigma_a_uniform, 0, 10);
    changed |= ImGui::SliderFloat("U Sigma S", &ptr->sigma_s_uniform, 0, 10);
    changed |= ImGui::SliderFloat("U G", &ptr->g_uniform, -0.999, 0.999);
    ImGui::SeparatorText("Perlin");
    changed |= ImGui::SliderFloat("P Sigma Maj", &ptr->sigma_maj_perlin, 0, 10);
    changed |= ImGui::SliderFloat("P Sigma A", &ptr->sigma_a_perlin, 0, 10);
    changed |= ImGui::SliderFloat("P Sigma S", &ptr->sigma_s_perlin, 0, 10);
    changed |= ImGui::SliderFloat("P G", &ptr->g_perlin, -0.999, 0.999);
    changed |= ImGui::SliderFloat("L", &ptr->L, 0, 4);
    changed |= ImGui::SliderFloat("H", &ptr->H, 0, 1);
    changed |= ImGui::SliderFloat("Freq", &ptr->freq, 0, 2);
    changed |= ImGui::SliderInt("OCT", &ptr->OCT, 0, 10);
    ImGui::SeparatorText("Cloud");
    changed |= ImGui::SliderFloat("C Sigma Maj", &ptr->sigma_maj_cloud, 0, 20);
    changed |= ImGui::SliderFloat("C Sigma A", &ptr->sigma_a_cloud, 0, 10);
    changed |= ImGui::SliderFloat("C Sigma S", &ptr->sigma_s_cloud, 0, 10);
    changed |= ImGui::SliderFloat("C G", &ptr->g_cloud, -0.999, 0.999);
    ImGui::SeparatorText("Smoke");
    changed |= ImGui::SliderFloat("S Sigma Maj", &ptr->sigma_maj_smoke, 0, 30);
    changed |= ImGui::SliderFloat("S Sigma A", &ptr->sigma_a_smoke, 0, 20);
    changed |= ImGui::SliderFloat("S Sigma S", &ptr->sigma_s_smoke, 0, 10);
    changed |= ImGui::SliderFloat("S G", &ptr->g_smoke, -0.999, 0.999);


    if (changed) {
        frame_count = 0;
    }
}

void VolumeRenderingScene::destroy() {
}
