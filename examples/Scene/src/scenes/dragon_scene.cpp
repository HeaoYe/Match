#include "dragon_scene.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include <imgui.h>

void DragonScene::initialize() {
    renderer->set_clear_value(Match::SWAPCHAIN_IMAGE_ATTACHMENT, { 0.3f, 0.5f, 0.7f, 1.0f });

    auto vert_shader = factory->load_shader("dragon_shader/shader.vert", Match::ShaderType::eVertexShaderNeedCompile);
    vert_shader->bind_descriptors({
        { 0, Match::DescriptorType::eUniform },
        { 1, Match::DescriptorType::eUniform },
    });
    auto frag_shader = factory->load_shader("dragon_shader/shader.frag", Match::ShaderType::eFragmentShaderNeedCompile);
    shader_program = factory->create_shader_program(renderer, "main");
    shader_program->attach_vertex_shader(vert_shader, "main");
    shader_program->attach_fragment_shader(frag_shader, "main");
    auto vas = factory->create_vertex_attribute_set({
        {
            .binding = 0,
            .rate = Match::InputRate::ePerVertex,
            .attributes = {
                Match::VertexType::eFloat3,  // in_pos
                Match::VertexType::eFloat3,  // in_normal
                Match::VertexType::eFloat3,  // in_color
            }
        },
        {
            .binding = 1,
            .rate = Match::InputRate::ePerInstance,
            .attributes = {
                Match::VertexType::eFloat3,  // offset
            },
        }
    });
    shader_program->bind_vertex_attribute_set(vas);
    shader_program->compile({
        .cull_mode = Match::CullMode::eBack,
        .front_face = Match::FrontFace::eClockwise,
        .depth_test_enable = VK_TRUE,
    });

    camera = std::make_unique<Camera>(factory);
    shader_program->bind_uniforms(0, { camera->uniform });

    light = std::make_unique<LightController>(factory);
    light->data.lights[0].pos = { 1, 1, 1 };
    light->data.lights[0].color = { 1, 1, 1 };
    light->data.num = 1;
    light->update();
    shader_program->bind_uniforms(1, { light->uniform });

    int n = 8, n2 = n / 2;
    offsets.reserve(n * n * n);
    for (int i = 0; i < n * n * n; i ++) {
        offsets.push_back({ i % n - n2, (i / n) % n - n2, ((i / n) / n) % n - n2 });
    }

    camera->data.pos = { 0, 0, -n - 5 };
    camera->upload_data();

    load_model("dragon.obj");
    vertex_buffer = factory->create_vertex_buffer(sizeof(Vertex), vertices.size());
    memcpy(vertex_buffer->map(), vertices.data(), sizeof(Vertex) * vertices.size());
    vertex_buffer->flush();
    offset_buffer = factory->create_vertex_buffer(sizeof(glm::vec3), offsets.size());
    memcpy(offset_buffer->map(), offsets.data(), sizeof(glm::vec3) * offsets.size());
    offset_buffer->flush();
    index_buffer = factory->create_index_buffer(Match::IndexType::eUint32, indices.size());
    memcpy(index_buffer->map(), indices.data(), sizeof(uint32_t) * indices.size());
    index_buffer->flush();
}

void DragonScene::update(float delta) {
    camera->update(delta);

    static float time = 0;
    light->data.lights[0].pos = glm::rotateY(glm::vec3(1.0f, 1.0f, 1.0f), time);
    light->update();
    time += delta * 3;
}

void DragonScene::render() {
    renderer->bind_shader_program(shader_program);
    renderer->bind_vertex_buffers({ vertex_buffer, offset_buffer });
    renderer->bind_index_buffer(index_buffer);
    renderer->draw_indexed(indices.size(), offsets.size(), 0, 0, 0);
}

void DragonScene::render_imgui() {
    ImGui::Text("Vertex Count: %d", (int) vertices.size());
    ImGui::Text("Index Count: %d", (int) indices.size());
    // 每一个龙有87w个三角形
    ImGui::Text("Triangle Count: %d", (int) (indices.size() / 3));
    // 512个龙共有4.4亿个三角形
    ImGui::Text("All Triangle Count: %d", (int) ((indices.size() / 3) * offsets.size()));
    ImGui::Text("Current FPS: %f", ImGui::GetIO().Framerate);

    ImGui::Separator();

    ImGui::ColorEdit3("LightColor", &light->data.lights[0].color.x);
}

void DragonScene::destroy() {
}

LightController::LightController(std::weak_ptr<Match::ResourceFactory> factory) {
    uniform = factory.lock()->create_uniform_buffer(sizeof(LightUniform));
}

void LightController::update() {
    memcpy(uniform->get_uniform_ptr(), &data, sizeof(LightUniform));
}
