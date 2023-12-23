#include "scene.hpp"
#include <tiny_obj_loader.h>
#include <imgui.h>
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/gtx/rotate_vector.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

Scene::Scene(std::shared_ptr<Match::ResourceFactory> factory, std::shared_ptr<Match::Renderer> renderer) : factory(factory), renderer(renderer) {
    renderer->set_clear_value(Match::SWAPCHAIN_IMAGE_ATTACHMENT, { 0.3f, 0.5f, 0.7f, 1.0f });

    auto vert_shader = factory->load_shader("shader.vert", Match::ShaderType::eVertexShaderNeedCompile);
    vert_shader->bind_descriptors({
        { 0, Match::DescriptorType::eUniform },
        { 1, Match::DescriptorType::eUniform },
    });
    auto frag_shader = factory->load_shader("shader.frag", Match::ShaderType::eFragmentShaderNeedCompile);
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
    camera->update();

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

void Scene::update(float delta) {
    auto d = glm::rotateY(glm::vec3(0.0f, 0.0f, 1.0f), glm::radians(camera->yaw));
    auto cd = glm::rotateY(d, glm::radians(90.0f));
    float speed = 1 * delta;
    if (glfwGetKey(Match::window->get_glfw_window(), GLFW_KEY_W))
        camera->data.pos += d * speed;
    if (glfwGetKey(Match::window->get_glfw_window(), GLFW_KEY_S))
        camera->data.pos -= d * speed;
    if (glfwGetKey(Match::window->get_glfw_window(), GLFW_KEY_A))
        camera->data.pos += cd * speed;
    if (glfwGetKey(Match::window->get_glfw_window(), GLFW_KEY_D))
        camera->data.pos -= cd * speed;
    if (glfwGetKey(Match::window->get_glfw_window(), GLFW_KEY_Q))
        camera->data.pos.y += speed;
    if (glfwGetKey(Match::window->get_glfw_window(), GLFW_KEY_E))
        camera->data.pos.y -= speed;
    camera->update();
    static float time = 0;
    light->data.lights[0].pos = glm::rotateY(glm::vec3(1.0f, 1.0f, 1.0f), time);
    // light->update();
    time += delta * 1.5;
}

void Scene::render() {
    renderer->bind_shader_program(shader_program);
    renderer->bind_vertex_buffers({ vertex_buffer, offset_buffer });
    renderer->bind_index_buffer(index_buffer);
    renderer->draw_indexed(indices.size(), offsets.size(), 0, 0, 0);
}

void Scene::render_imgui() {
    ImGui::Text("Vertex Count: %d", (int) vertices.size());
    ImGui::Text("Index Count: %d", (int) indices.size());
    // 每一个龙有87w个三角形
    ImGui::Text("Triangle Count: %d", (int) (indices.size() / 3));
    // 512个龙共有4.4亿个三角形
    ImGui::Text("All Triangle Count: %d", (int) ((indices.size() / 3) * offsets.size()));
    ImGui::Text("Current FPS: %f", ImGui::GetIO().Framerate);

    ImGui::Separator();

    ImGui::ColorEdit3("LightColor", &light->data.lights[0].color.x);
    light->update();
}

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.in_pos) ^
                   (hash<glm::vec3>()(vertex.in_normal) << 1)) >> 1) ^
                   (hash<glm::vec3>()(vertex.in_color) << 1);
        }
    };
}

void Scene::load_model(const std::string &filename) {
    auto path = "resource/models/" + filename;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> unique_vertices;
    uint32_t count = 0;
    for (const auto& shape : shapes) {
        count += shape.mesh.indices.size();
    }
    vertices.reserve(count);
    indices.reserve(count);

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex {};
            vertex.in_pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            vertex.in_normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };
            vertex.in_color = { 1, 1, 1 };

            if (unique_vertices.count(vertex) == 0) {
                unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(unique_vertices[vertex]);
        }
    }
}

Camera::Camera(std::weak_ptr<Match::ResourceFactory> factory) {
    uniform = factory.lock()->create_uniform_buffer(sizeof(CameraUniform));
    yaw = 0;
    pitch = 0;
    glfwSetWindowUserPointer(Match::window->get_glfw_window(), this);

    glfwSetKeyCallback(Match::window->get_glfw_window(), [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto camera = (Camera *)glfwGetWindowUserPointer(window);
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            glfwSetInputMode(Match::window->get_glfw_window(), GLFW_CURSOR, camera->flag ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            camera->flag = !camera->flag;
        }
    });
    glfwSetCursorPosCallback(Match::window->get_glfw_window(), [](GLFWwindow* window, double xpos, double ypos) {
        static double last_x, last_y;
        static bool first = true;
        auto camera = (Camera *)glfwGetWindowUserPointer(window);
        ImGui::GetIO().AddMousePosEvent(xpos, ypos);
        if (camera->flag) {
            last_x = xpos;
            last_y = ypos;
            return;
        }
        if (!first) {
            double dt = ImGui::GetIO().DeltaTime;
            static double speed = 7;
            double delta_x = (xpos - last_x) * dt * speed;
            double delta_y = (ypos - last_y) * dt * speed;

            camera->yaw -= delta_x;
            camera->pitch = std::clamp<float>(camera->pitch + delta_y, -89.0f, 89.0f);
        } else {
            first = false;
        }
        last_x = xpos;
        last_y = ypos;
        camera->update();
    });
}

void Camera::update() {
    data.direction = glm::rotateY(glm::rotateX(glm::vec3(0, 0, 1), glm::radians(pitch)), glm::radians(yaw));
    data.view = glm::lookAt(data.pos, data.pos + data.direction, glm::vec3(.0f, 1.0f, .0f));
    data.project = glm::perspective(glm::radians(60.0f), (float) Match::setting.window_size[0] / (float) Match::setting.window_size[1], 0.1f, 100.0f);
    memcpy(uniform->get_uniform_ptr(), &data, sizeof(CameraUniform));
}

LightController::LightController(std::weak_ptr<Match::ResourceFactory> factory) {
    uniform = factory.lock()->create_uniform_buffer(sizeof(LightUniform));
}

void LightController::update() {
    memcpy(uniform->get_uniform_ptr(), &data, sizeof(LightUniform));
}
