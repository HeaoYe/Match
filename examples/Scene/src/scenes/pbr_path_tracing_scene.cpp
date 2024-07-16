#include "pbr_path_tracing_scene.hpp"

void PBRPathTracingScene::initialize() {
    this->is_ray_tracing_scene = true;
    auto render_pass_builder = factory->create_render_pass_builder();
    render_pass_builder->add_subpass("main")
        .attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
    renderer = factory->create_renderer(render_pass_builder);
    renderer->attach_render_layer<Match::ImGuiLayer>("imgui layer");

    camera = std::make_unique<Camera>(*factory);
    camera->data.pos = { 0, 0, -3 };
    camera->upload_data();

    // cornell box gltf
    gltf_scene = factory->load_gltf_scene("cornell_box/scene.gltf", { "NORMAL" });
    auto builder = factory->create_acceleration_structure_builder();
    builder->add_scene(gltf_scene);
    builder->build();
    builder.reset();

    instance_collect = factory->create_ray_tracing_instance_collect();
    instance_collect->register_custom_instance_data<Match::GLTFPrimitiveInstanceData>();
    instance_collect->add_scene(0, gltf_scene, 0);
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

    auto raygen_shader = factory->compile_shader("pbr_path_tracing_shader/rt.rgen", Match::ShaderStage::eRaygen);
    auto miss_shader = factory->compile_shader("pbr_path_tracing_shader/rt.rmiss", Match::ShaderStage::eMiss);
    auto closest_hit_shader = factory->compile_shader("pbr_path_tracing_shader/rt.rchit", Match::ShaderStage::eClosestHit);

    ray_tracing_shader_program_constants = factory->create_push_constants(
        Match::ShaderStage::eRaygen | Match::ShaderStage::eClosestHit,
        {
            { "time", Match::ConstantType::eFloat },
            { "sample_count", Match::ConstantType::eInt32 },
            { "frame_count", Match::ConstantType::eInt32 },
            { "roughness", Match::ConstantType::eFloat },
            { "metallic", Match::ConstantType::eFloat },
            { "F0", Match::ConstantType::eFloat3 },
            { "light_intensity", Match::ConstantType::eFloat },
            { "prob", Match::ConstantType::eFloat },
        }
    );

    ray_tracing_shader_program_ds = factory->create_descriptor_set(renderer);
    ray_tracing_shader_program_ds->add_descriptors({
        { Match::ShaderStage::eRaygen, 0, Match::DescriptorType::eStorageImage },
        { Match::ShaderStage::eRaygen | Match::ShaderStage::eClosestHit, 1, Match::DescriptorType::eRayTracingInstance },
        { Match::ShaderStage::eRaygen, 2, Match::DescriptorType::eUniform },
        { Match::ShaderStage::eClosestHit, 3, Match::DescriptorType::eStorageBuffer },
        { Match::ShaderStage::eClosestHit, 4, Match::DescriptorType::eStorageBuffer },    // GLTFPrimitive信息
        { Match::ShaderStage::eClosestHit, 5, Match::DescriptorType::eStorageBuffer },    // 材质信息
        { Match::ShaderStage::eClosestHit, 6, Match::DescriptorType::eStorageBuffer },    // 法线缓存
    })
        .allocate()
        .bind_storage_image(0, ray_tracing_result_image)
        .bind_ray_tracing_instance_collect(1, instance_collect)
        .bind_uniform(2, camera->uniform)
        .bind_storage_buffer(3, instance_collect->get_instance_address_data_buffer())
        .bind_storage_buffer(4, instance_collect->get_custom_instance_data_buffer<Match::GLTFPrimitiveInstanceData>())
        .bind_storage_buffer(5, gltf_scene->get_materials_buffer())
        .bind_storage_buffer(6, gltf_scene->get_attribute_buffer("NORMAL"));

    ray_tracing_shader_program = factory->create_ray_tracing_shader_program();
    ray_tracing_shader_program->attach_raygen_shader(raygen_shader)
        .attach_miss_shader(miss_shader)
        .attach_hit_group(closest_hit_shader)
        .attach_descriptor_set(ray_tracing_shader_program_ds)
        .attach_push_constants(ray_tracing_shader_program_constants)
        .compile({
            .max_ray_recursion_depth = 1,
        });

    auto vert_shader = factory->compile_shader("pbr_path_tracing_shader/shader.vert", Match::ShaderStage::eVertex);
    auto frag_shader = factory->compile_shader("pbr_path_tracing_shader/shader.frag", Match::ShaderStage::eFragment);

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

void PBRPathTracingScene::update(float dt) {
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

void PBRPathTracingScene::render() {
    renderer->bind_shader_program(ray_tracing_shader_program);
    renderer->trace_rays();

    renderer->begin_render_pass();
    renderer->bind_shader_program(shader_program);
    auto [width, height] = Match::runtime_setting->get_window_size();
    renderer->set_viewport(0, static_cast<float>(height), static_cast<float>(width), -static_cast<float>(height));
    renderer->set_scissor(0, 0, width, height);
    renderer->draw(3, 1, 0, 0);
}

void PBRPathTracingScene::render_imgui() {
    ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);

    bool changed = false;

    static int sample_count = 3;
    changed |= ImGui::SliderInt("Sample Count", &sample_count, 1, 16);
    ray_tracing_shader_program_constants->push_constant("sample_count", sample_count);

    static float roughness = 1;
    changed |= ImGui::SliderFloat("Roughness", &roughness, 0, 1);
    ray_tracing_shader_program_constants->push_constant("roughness", roughness);

    static float metallic = 0;
    changed |= ImGui::SliderFloat("Metallic", &metallic, 0, 1);
    ray_tracing_shader_program_constants->push_constant("metallic", metallic);

    static glm::vec3 F0 { 0.04 };
    changed |= ImGui::ColorEdit3("F0", &F0.r);
    ray_tracing_shader_program_constants->push_constant("F0", &F0.r);

    static float light_intensity = 3;
    changed |= ImGui::SliderFloat("Light Intensity", &light_intensity, 1, 100);
    ray_tracing_shader_program_constants->push_constant("light_intensity", light_intensity);

    static float prob = 0.8;
    changed |= ImGui::SliderFloat("Prob", &prob, 0.1, 0.9);
    ray_tracing_shader_program_constants->push_constant("prob", prob);

    if (changed) {
        frame_count = 0;
    }
}

void PBRPathTracingScene::destroy() {
}
