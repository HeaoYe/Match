#include "microfacet_theory_scene.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

void MicrofacetTheoryScene::initialize() {
    // 设置日志级别
    Match::set_log_level(Match::LogLevel::eDebug);

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
    sphere_collect->register_custom_sphere_data<MicrofacetTheoryMaterial>();

    // 介电质
    dielectric_material = {
        .albedo = { 216. / 255., 148. / 255., 235. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 2,
        .K = 0,
        .alpha_x = 0.5,
        .alpha_y = 0.05,
    };
    sphere_collect->add_sphere(0, { -1, 1, 0 }, 0.9, dielectric_material);
    // 导体
    conductor_material = {
        .albedo = { 189. / 255., 143. / 255., 198. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eConductor,
        .IOR = 2,
        .K = 3,
        .alpha_x = 0.05,
        .alpha_y = 0.5,
    };
    sphere_collect->add_sphere(0, { 1, 1, 0 }, 0.9, conductor_material);
    // 大地
    sphere_collect->add_sphere(0, {0, -500, 0 }, 500, MicrofacetTheoryMaterial { .albedo = { 0.8, 0.5, 0.25 } });

    bunny1_material = {
        .albedo = { 231. / 255., 235. / 255., 173. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eConductor,
        .IOR = 1.5,
        .K = 3,
        .alpha_x = 0,
        .alpha_y = 0.2,
    };
    bunny2_material = {
        .albedo = { 131. / 255., 182. / 255., 224. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 2.5,
        .K = 1,
        .alpha_x = 0.1,
        .alpha_y = 0.1,
    };
    dragon_material = {
        .albedo = { 108. / 255., 129. / 255., 78. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 1.5,
        .K = 1,
        .alpha_x = 0.1,
        .alpha_y = 0.1,
    };
    outter1_material = {
        .albedo = { 222. / 255., 91. / 255., 36. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eConductor,
        .IOR = 2,
        .K = 3,
        .alpha_x = 0.02,
        .alpha_y = 0.02,
    };
    inner1_material = {
        .albedo = { 1, 1, 1 },
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 4,
        .K = 0,
        .alpha_x = 0.4,
        .alpha_y = 0.4,
    };
    outter2_material = {
        .albedo = { 222. / 255., 91. / 255., 36. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 2,
        .K = 3,
        .alpha_x = 0.02,
        .alpha_y = 0.02,
    };
    inner2_material = {
        .albedo = { 1, 1, 1 },
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 4,
        .K = 0,
        .alpha_x = 0.4,
        .alpha_y = 0.4,
    };
    model_bunny = factory->load_model("bunny.obj");
    model_dragon = factory->load_model("dragon.obj");
    model_outter = factory->load_model("mitsuba.obj", { "1", "2" });
    model_inner = factory->load_model("mitsuba.obj", { "1", "3" });

    auto builder = factory->create_acceleration_structure_builder();
    builder->add_model(sphere_collect);
    builder->add_model(model_bunny);
    builder->add_model(model_dragon);
    builder->add_model(model_outter);
    builder->add_model(model_inner);
    builder->build();
    builder.reset();

    instance_collect = factory->create_ray_tracing_instance_collect();
    instance_collect->register_custom_instance_data<MicrofacetTheoryMaterial>();
    instance_collect->add_instance(0, sphere_collect, glm::mat4(1), 0);
    instance_collect->add_instance(1, model_bunny, glm::translate(glm::vec3(-1.7, 0.1, 3)), 1, bunny1_material);
    instance_collect->add_instance(1, model_dragon, glm::translate(glm::vec3(0.2, 0.8, 6)) * glm::rotate(glm::radians(90.f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(2.5, 2.5, 2.5)), 1, dragon_material);
    instance_collect->add_instance(1, model_bunny, glm::translate(glm::vec3(2, 0.1, 3)), 1, bunny2_material);

    instance_collect->add_instance(1, model_outter, glm::translate(glm::vec3(-2.5, 0.1, 6)), 1, outter1_material);
    instance_collect->add_instance(1, model_inner, glm::translate(glm::vec3(-2.5, 0.1, 6)), 1, inner1_material);
    instance_collect->add_instance(1, model_outter, glm::translate(glm::vec3(2.8, 0.1, 6)), 1, outter2_material);
    instance_collect->add_instance(1, model_inner, glm::translate(glm::vec3(2.8, 0.1, 6)), 1, inner2_material);
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

    auto raygen_shader = factory->compile_shader("microfacet_theory_shader/rt.rgen", Match::ShaderStage::eRaygen);
    auto miss_shader = factory->compile_shader("microfacet_theory_shader/rt.rmiss", Match::ShaderStage::eMiss);
    auto closest_hit_shader = factory->compile_shader("microfacet_theory_shader/sphere.rchit", Match::ShaderStage::eClosestHit);
    auto intersection_shader = factory->compile_shader("microfacet_theory_shader/sphere.rint", Match::ShaderStage::eIntersection);
    auto triangle_closest_hit_shader = factory->compile_shader("microfacet_theory_shader/triangle.rchit", Match::ShaderStage::eClosestHit);

    ray_tracing_shader_program_constants = factory->create_push_constants(
        Match::ShaderStage::eRaygen,
        {
            { "time", Match::ConstantType::eFloat },
            { "sample_count", Match::ConstantType::eInt32 },
            { "frame_count", Match::ConstantType::eInt32 },
            { "view_depth", Match::ConstantType::eFloat },
            { "view_depth_strength", Match::ConstantType::eFloat },
            { "prob", Match::ConstantType::eFloat },
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
        { Match::ShaderStage::eClosestHit, 6, Match::DescriptorType::eStorageBuffer },
    })
        .allocate()
        .bind_storage_image(0, ray_tracing_result_image)
        .bind_ray_tracing_instance_collect(1, instance_collect)
        .bind_uniform(2, camera->uniform)
        .bind_storage_buffer(3, sphere_collect->get_spheres_buffer())
        .bind_storage_buffer(4, sphere_collect->get_custom_data_buffer<MicrofacetTheoryMaterial>())
        .bind_storage_buffer(5, instance_collect->get_instance_address_data_buffer())
        .bind_storage_buffer(6, instance_collect->get_custom_instance_data_buffer<MicrofacetTheoryMaterial>());

    ray_tracing_shader_program = factory->create_ray_tracing_shader_program();
    ray_tracing_shader_program->attach_raygen_shader(raygen_shader)
        .attach_miss_shader(miss_shader)
        .attach_hit_group(closest_hit_shader, intersection_shader)
        .attach_hit_group(triangle_closest_hit_shader)
        .attach_descriptor_set(ray_tracing_shader_program_ds)
        .attach_push_constants(ray_tracing_shader_program_constants)
        .compile({
            .max_ray_recursion_depth = 1,  // 因为提取了光线递归,所以最大深度为1,这样可以加速光追渲染
        });

    auto vert_shader = factory->compile_shader("microfacet_theory_shader/shader.vert", Match::ShaderStage::eVertex);
    auto frag_shader = factory->compile_shader("microfacet_theory_shader/shader.frag", Match::ShaderStage::eFragment);

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

void MicrofacetTheoryScene::update(float dt) {
    camera->update(dt);

    static float time = 0;
    time += dt;
    ray_tracing_shader_program_constants->push_constant("time", time);

    if (camera->is_capture_cursor) {
        frame_count = 0;
    }
    ray_tracing_shader_program_constants->push_constant("frame_count", frame_count);
    frame_count ++;

    sphere_collect->update<MicrofacetTheoryMaterial>(0, [this](uint32_t index, auto &material) {
        if (index == 0) {
            material = dielectric_material;
        } else if (index == 1) {
            material = conductor_material;
        }
    });
    instance_collect->update<MicrofacetTheoryMaterial>(1, [this](uint32_t index, auto &material) {
        if (index == 0) {
            material = bunny1_material;
        } else if (index == 1) {
            material = dragon_material;
        } else if (index == 2) {
            material = bunny2_material;
        } else if (index == 3) {
            material = outter1_material;
        } else if (index == 4) {
            material = inner1_material;
        } else if (index == 5) {
            material = outter2_material;
        } else if (index == 6) {
            material = inner2_material;
        }
    });
}

void MicrofacetTheoryScene::render() {
    renderer->bind_shader_program(ray_tracing_shader_program);
    renderer->trace_rays();

    renderer->begin_render_pass();
    renderer->bind_shader_program(shader_program);
    auto [width, height] = Match::runtime_setting->get_window_size();
    renderer->set_viewport(0, static_cast<float>(height), static_cast<float>(width), -static_cast<float>(height));
    renderer->set_scissor(0, 0, width, height);
    renderer->draw(3, 1, 0, 0);
}

void MicrofacetTheoryScene::render_imgui() {
    ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);

    bool changed = false;

    static int sample_count = 3;
    changed |= ImGui::SliderInt("Sample Count", &sample_count, 1, 16);
    ray_tracing_shader_program_constants->push_constant("sample_count", sample_count);

    static float prob = 0.8;
    changed |= ImGui::SliderFloat("Prob", &prob, 0.1, 0.9);
    ray_tracing_shader_program_constants->push_constant("prob", prob);

    static float view_depth = 1;
    changed |= ImGui::SliderFloat("View Depth", &view_depth, 0.01, 5);
    changed |= ImGui::SliderFloat("View Depth High", &view_depth, 5, 100);
    ray_tracing_shader_program_constants->push_constant("view_depth", view_depth);

    static float view_depth_strength = 0;
    changed |= ImGui::SliderFloat("View Depth Strength", &view_depth_strength, 0, 0.5);
    ray_tracing_shader_program_constants->push_constant("view_depth_strength", view_depth_strength);

    ImGui::SeparatorText("Dielectric");
    changed |= ImGui::ColorEdit3("D Albedo", &dielectric_material.albedo.r);
    changed |= ImGui::SliderFloat("D IOR", &dielectric_material.IOR, 1, 5);
    changed |= ImGui::SliderFloat("D Alpha X", &dielectric_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("D Alpha Y", &dielectric_material.alpha_y, 0, 1);

    ImGui::SeparatorText("Conductor");
    changed |= ImGui::ColorEdit3("C Albedo", &conductor_material.albedo.r);
    changed |= ImGui::SliderFloat("C IOR", &conductor_material.IOR, 1, 5);
    changed |= ImGui::SliderFloat("C K", &conductor_material.K, 0, 5);
    changed |= ImGui::SliderFloat("C Alpha X", &conductor_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("C Alpha Y", &conductor_material.alpha_y, 0, 1);

    ImGui::SeparatorText("Bunny 1");
    changed |= ImGui::ColorEdit3("B1 Albedo", &bunny1_material.albedo.r);
    changed |= ImGui::Checkbox("B1 Is Conductor", reinterpret_cast<bool *>(&bunny1_material.type));
    changed |= ImGui::SliderFloat("B1 IOR", &bunny1_material.IOR, 1, 5);
    changed |= ImGui::SliderFloat("B1 K", &bunny1_material.K, 0, 5);
    changed |= ImGui::SliderFloat("B1 Alpha X", &bunny1_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("B1 Alpha Y", &bunny1_material.alpha_y, 0, 1);
    ImGui::SeparatorText("Bunny 2");
    changed |= ImGui::ColorEdit3("B2 Albedo", &bunny2_material.albedo.r);
    changed |= ImGui::Checkbox("B2 Is Conductor", reinterpret_cast<bool *>(&bunny2_material.type));
    changed |= ImGui::SliderFloat("B2 IOR", &bunny2_material.IOR, 1, 5);
    changed |= ImGui::SliderFloat("B2 K", &bunny2_material.K, 0, 5);
    changed |= ImGui::SliderFloat("B2 Alpha X", &bunny2_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("B2 Alpha Y", &bunny2_material.alpha_y, 0, 1);
    ImGui::SeparatorText("Dragon");
    changed |= ImGui::ColorEdit3("Dr Albedo", &dragon_material.albedo.r);
    changed |= ImGui::Checkbox("Dr Is Conductor", reinterpret_cast<bool *>(&dragon_material.type));
    changed |= ImGui::SliderFloat("Dr IOR", &dragon_material.IOR, 1, 5);
    changed |= ImGui::SliderFloat("Dr K", &dragon_material.K, 0, 5);
    changed |= ImGui::SliderFloat("Dr Alpha X", &dragon_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("Dr Alpha Y", &dragon_material.alpha_y, 0, 1);

    ImGui::SeparatorText("Outter 1");
    changed |= ImGui::ColorEdit3("O1 Albedo", &outter1_material.albedo.r);
    changed |= ImGui::Checkbox("O1 Is Conductor", reinterpret_cast<bool *>(&outter1_material.type));
    changed |= ImGui::SliderFloat("O1 IOR", &outter1_material.IOR, 1, 5);
    changed |= ImGui::SliderFloat("O1 K", &outter1_material.K, 0, 5);
    changed |= ImGui::SliderFloat("O1 Alpha X", &outter1_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("O1 Alpha Y", &outter1_material.alpha_y, 0, 1);

    ImGui::SeparatorText("Inner 1");
    changed |= ImGui::ColorEdit3("I1 Albedo", &inner1_material.albedo.r);
    changed |= ImGui::Checkbox("I1 Is Conductor", reinterpret_cast<bool *>(&inner1_material.type));
    changed |= ImGui::SliderFloat("I1 IOR", &inner1_material.IOR, 1, 5);
    changed |= ImGui::SliderFloat("I1 K", &inner1_material.K, 0, 5);
    changed |= ImGui::SliderFloat("I1 Alpha X", &inner1_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("I1 Alpha Y", &inner1_material.alpha_y, 0, 1);

    ImGui::SeparatorText("Outter 2");
    changed |= ImGui::ColorEdit3("O2 Albedo", &outter2_material.albedo.r);
    changed |= ImGui::Checkbox("O2 Is Conductor", reinterpret_cast<bool *>(&outter2_material.type));
    changed |= ImGui::SliderFloat("O2 IOR", &outter2_material.IOR, 1, 5);
    changed |= ImGui::SliderFloat("O2 K", &outter2_material.K, 0, 5);
    changed |= ImGui::SliderFloat("O2 Alpha X", &outter2_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("O2 Alpha Y", &outter2_material.alpha_y, 0, 1);

    ImGui::SeparatorText("Inner 2");
    changed |= ImGui::ColorEdit3("I2 Albedo", &inner2_material.albedo.r);
    changed |= ImGui::Checkbox("I2 Is Conductor", reinterpret_cast<bool *>(&inner2_material.type));
    changed |= ImGui::SliderFloat("I2 IOR", &inner2_material.IOR, 1, 5);
    changed |= ImGui::SliderFloat("I2 K", &inner2_material.K, 0, 5);
    changed |= ImGui::SliderFloat("I2 Alpha X", &inner2_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("I2 Alpha Y", &inner2_material.alpha_y, 0, 1);

    if (changed) {
        frame_count = 0;
    }
}

void MicrofacetTheoryScene::destroy() {
}
