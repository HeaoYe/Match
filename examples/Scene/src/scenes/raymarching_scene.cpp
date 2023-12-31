#include "raymarching_scene.hpp"
#include <glm/gtx/rotate_vector.hpp>

void RayMarchingScene::initialize() {
    auto builder = factory->create_render_pass_builder();
    auto &main_subpass = builder->add_subpass("main");
    main_subpass.attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
    renderer = factory->create_renderer(builder);
    renderer->attach_render_layer<Match::ImGuiLayer>("imgui layer");
    
    auto vert_shader = factory->load_shader("raymarching_shader/shader.vert", Match::ShaderType::eVertexShaderNeedCompile);
    auto frag_shader = factory->load_shader("raymarching_shader/shader.frag", Match::ShaderType::eFragmentShaderNeedCompile);
    frag_shader->bind_descriptors({
        { 0, Match::DescriptorType::eUniform },
        { 1, Match::DescriptorType::eUniform },
    });
    shader_program = factory->create_shader_program(renderer, "main");
    shader_program->attach_vertex_shader(vert_shader);
    shader_program->attach_fragment_shader(frag_shader);
    shader_program->compile({
        .cull_mode = Match::CullMode::eNone,
        .depth_test_enable = VK_FALSE,
    });

    camera = std::make_unique<Camera>(*factory);
    camera->data.pos = { 0, 1, 0 };
    camera->upload_data();
    shader_program->bind_uniforms(0, { camera->uniform });
    uniform_buffer = factory->create_uniform_buffer(sizeof(RayMarchingUniform));
    data = static_cast<RayMarchingUniform *>(uniform_buffer->get_uniform_ptr());
    shader_program->bind_uniforms(1, { uniform_buffer });
    data->max_steps = 100;
    data->max_dist = 100.0f;
    data->epsillon_dist = 0.00001f;
    data->sphere = { 0, 1, 2, 1 };
}

void RayMarchingScene::update(float dt) {
    camera->update(dt);
    auto usize = Match::runtime_setting->get_window_size();
    data->window_size = { usize.width, usize.height };
    static float time = 0;
    time += dt;
    data->light = glm::vec3(data->sphere.x, 0, data->sphere.z) + glm::rotateY(glm::vec3(1, 1, 1), time);
}

void RayMarchingScene::render() {
    renderer->bind_shader_program(shader_program);
    renderer->draw(3, 2, 0, 0);
}

void RayMarchingScene::render_imgui() {
    ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);

    ImGui::SliderInt("Max Steps", &data->max_steps, 1, 1000);
    ImGui::SliderFloat("Max Dist", &data->max_dist, 0.001, 1000);
    ImGui::SliderFloat("Epsillon Dist", &data->epsillon_dist, 0.000001, 0.01);
    ImGui::SliderFloat3("Sphere Pos", &data->sphere.x, -3, 3);
    ImGui::SliderFloat("Sphere R", &data->sphere.w, 0.001, 2);
}

void RayMarchingScene::destroy() {
}

