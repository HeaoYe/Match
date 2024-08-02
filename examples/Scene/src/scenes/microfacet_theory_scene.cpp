#include "microfacet_theory_scene.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

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
    camera->data.pos = { 0, 1, -3 };
    camera->upload_data();

    sphere_collect = factory->create_sphere_collect();
    sphere_collect->register_custom_sphere_data<MicrofacetTheoryMaterial>();

    // 电介质
    dielectric_material = {
        .albedo = { 216. / 255., 148. / 255., 235. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 2,
        .K = 0,
        .alpha_x = 0.5,
        .alpha_y = 0.05,
    };
    sphere_collect->add_sphere(0, { -1.5, 1, 0 }, 0.9, dielectric_material);

    // 导体
    conductor_material = {
        .albedo = { 189. / 255., 143. / 255., 198. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eConductor,
        .IOR = 2,
        .K = 3.5,
        .alpha_x = 0.05,
        .alpha_y = 0.5,
    };
    sphere_collect->add_sphere(0, { 1.5, 1, 0 }, 0.9, conductor_material);

    // 大地
    sphere_collect->add_sphere(0, {0, -500, 0 }, 500, MicrofacetTheoryMaterial { .albedo = { 0.8, 0.5, 0.25 } });

    bunny_d_material = {
        .albedo = { 131. / 255., 182. / 255., 224. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 2.5,
        .K = 0,
        .alpha_x = 0.1,
        .alpha_y = 0.1,
    };
    bunny_dt_material = {
        .albedo = { 210. / 255., 115. / 255., 211. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 1.2,
        .K = 0,
        .alpha_x = 0,
        .alpha_y = 0,
    };
    bunny_c_material = {
        .albedo = { 231. / 255., 235. / 255., 173. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eConductor,
        .IOR = 1.5,
        .K = 3,
        .alpha_x = 0,
        .alpha_y = 0.2,
    };
    model_bunny = factory->load_model("bunny.obj");

    outter_d_material = {
        .albedo = { 222. / 255., 91. / 255., 36. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 2,
        .K = 0,
        .alpha_x = 0.02,
        .alpha_y = 0.02,
    };
    outter_c_material = {
        .albedo = { 222. / 255., 91. / 255., 36. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eConductor,
        .IOR = 2,
        .K = 3,
        .alpha_x = 0.02,
        .alpha_y = 0.02,
    };
    inner_material = {
        .albedo = { 1, 1, 1 },
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 2.5,
        .K = 0,
        .alpha_x = 0.4,
        .alpha_y = 0.4,
    };
    model_outter = factory->load_model("mitsuba.obj", { "1", "2" });
    model_inner = factory->load_model("mitsuba.obj", { "1", "3" });

    dragon_material = {
        .albedo = { 108. / 255., 129. / 255., 78. / 255. },
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 1.5,
        .K = 0,
        .alpha_x = 0.1,
        .alpha_y = 0.1,
    };
    model_dragon = factory->load_model("dragon.obj");

    auto builder = factory->create_acceleration_structure_builder();
    builder->add_model(sphere_collect);
    builder->add_model(model_bunny);
    builder->add_model(model_outter);
    builder->add_model(model_inner);
    builder->add_model(model_dragon);
    builder->build();
    builder.reset();

    instance_collect = factory->create_ray_tracing_instance_collect();
    instance_collect->register_custom_instance_data<MicrofacetTheoryMaterial>();
    instance_collect->add_instance(0, sphere_collect, glm::mat4(1), 0);

    instance_collect->add_instance(1, model_bunny, glm::translate(glm::vec3(-2, 0.1, 3)), 1, bunny_d_material);
    instance_collect->add_instance(1, model_bunny, glm::translate(glm::vec3(0, 0.1, 3)), 1, bunny_dt_material);
    instance_collect->add_instance(1, model_bunny, glm::translate(glm::vec3(2, 0.1, 3)), 1, bunny_c_material);

    instance_collect->add_instance(1, model_outter, glm::translate(glm::vec3(-2.7, 0.1, 6)), 1, outter_d_material);
    instance_collect->add_instance(1, model_inner, glm::translate(glm::vec3(-2.7, 0.1, 6.1)), 1, inner_material);
    instance_collect->add_instance(1, model_outter, glm::translate(glm::vec3(2.7, 0.1, 6)), 1, outter_c_material);
    instance_collect->add_instance(1, model_inner, glm::translate(glm::vec3(2.7, 0.1, 6.1)), 1, inner_material);

    instance_collect->add_instance(1, model_dragon, glm::translate(glm::vec3(0.1, 0.8, 6)) * glm::rotate(glm::radians(90.f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(2.5f, 2.5f ,2.5f)), 1, inner_material);

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
            material = bunny_d_material;
        } else if (index == 1) {
            material = bunny_dt_material;
        } else if (index == 2) {
            material = bunny_c_material;
        } else if (index == 3) {
            material = outter_d_material;
        } else if (index == 4) {
            material = inner_material;
        } else if (index == 5) {
            material = outter_c_material;
        } else if (index == 6) {
            material = inner_material;
        } else if (index == 7) {
            material = dragon_material;
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

    static int sample_count = 4;
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

    ImGui::SeparatorText("Dielectric Sphere");
    changed |= ImGui::ColorEdit3("D Albedo", &dielectric_material.albedo.r);
    changed |= ImGui::SliderFloat("D IOR", &dielectric_material.IOR, 1, 3);
    changed |= ImGui::SliderFloat("D Alpha X", &dielectric_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("D Alpha Y", &dielectric_material.alpha_y, 0, 1);

    ImGui::SeparatorText("Conductor Sphere");
    changed |= ImGui::ColorEdit3("C Albedo", &conductor_material.albedo.r);
    changed |= ImGui::SliderFloat("C IOR", &conductor_material.IOR, 1, 3);
    changed |= ImGui::SliderFloat("C K", &conductor_material.K, 0, 5);
    changed |= ImGui::SliderFloat("C Alpha X", &conductor_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("C Alpha Y", &conductor_material.alpha_y, 0, 1);

    ImGui::SeparatorText("Bunny D");
    changed |= ImGui::ColorEdit3("BD Albedo", &bunny_d_material.albedo.r);
    changed |= ImGui::SliderFloat("BD IOR", &bunny_d_material.IOR, 1, 3);
    changed |= ImGui::SliderFloat("BD Alpha X", &bunny_d_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("BD Alpha Y", &bunny_d_material.alpha_y, 0, 1);

    ImGui::SeparatorText("Bunny DT");
    changed |= ImGui::ColorEdit3("BDT Albedo", &bunny_dt_material.albedo.r);
    changed |= ImGui::SliderFloat("BDT IOR", &bunny_dt_material.IOR, 1, 3);
    changed |= ImGui::SliderFloat("BDT Alpha X", &bunny_dt_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("BDT Alpha Y", &bunny_dt_material.alpha_y, 0, 1);

    ImGui::SeparatorText("Bunny C");
    changed |= ImGui::ColorEdit3("BC Albedo", &bunny_c_material.albedo.r);
    changed |= ImGui::SliderFloat("BC IOR", &bunny_c_material.IOR, 1, 3);
    changed |= ImGui::SliderFloat("BC K", &bunny_c_material.K, 0, 5);
    changed |= ImGui::SliderFloat("BC Alpha X", &bunny_c_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("BC Alpha Y", &bunny_c_material.alpha_y, 0, 1);

    ImGui::SeparatorText("Outter D");
    changed |= ImGui::ColorEdit3("OD Albedo", &outter_d_material.albedo.r);
    changed |= ImGui::SliderFloat("OD IOR", &outter_d_material.IOR, 1, 3);
    changed |= ImGui::SliderFloat("OD Alpha X", &outter_d_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("OD Alpha Y", &outter_d_material.alpha_y, 0, 1);

    ImGui::SeparatorText("Outter C");
    changed |= ImGui::ColorEdit3("OC Albedo", &outter_c_material.albedo.r);
    changed |= ImGui::SliderFloat("OC IOR", &outter_c_material.IOR, 1, 3);
    changed |= ImGui::SliderFloat("OC K", &outter_c_material.K, 0, 5);
    changed |= ImGui::SliderFloat("OC Alpha X", &outter_c_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("OC Alpha Y", &outter_c_material.alpha_y, 0, 1);

    ImGui::SeparatorText("Inner");
    changed |= ImGui::ColorEdit3("I Albedo", &inner_material.albedo.r);
    changed |= ImGui::SliderFloat("I IOR", &inner_material.IOR, 1, 3);
    changed |= ImGui::SliderFloat("I Alpha X", &inner_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("I Alpha Y", &inner_material.alpha_y, 0, 1);

    ImGui::SeparatorText("Dragon");
    changed |= ImGui::ColorEdit3("Dr Albedo", &dragon_material.albedo.r);
    changed |= ImGui::SliderFloat("Dr IOR", &dragon_material.IOR, 1, 3);
    changed |= ImGui::SliderFloat("Dr Alpha X", &dragon_material.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("Dr Alpha Y", &dragon_material.alpha_y, 0, 1);

    if (changed) {
        frame_count = 0;
    }
}

void MicrofacetTheoryScene::destroy() {
}
