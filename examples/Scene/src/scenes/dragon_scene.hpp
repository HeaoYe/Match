#pragma once

#include <Match/Match.hpp>
#include "scene.hpp"
#include "../camera.hpp"


struct PointLight {
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 color;
};

struct LightController {
    struct LightUniform {
        alignas(16) PointLight lights[10];
        alignas(4) uint32_t num;
    } *data;
    LightController(std::weak_ptr<Match::ResourceFactory> factory);
    std::shared_ptr<Match::UniformBuffer> uniform;
};

// 使用宏定义Scene
class DragonScene final : public Scene {
    define_scene(DragonScene)
private:
    // 场景资源
    std::unique_ptr<Camera> camera;
    std::unique_ptr<LightController> light;
    std::shared_ptr<Match::GraphicsShaderProgram> shader_program;
    std::shared_ptr<Match::GraphicsShaderProgram> post_shader_program;
    std::shared_ptr<Match::Model> model;
    std::vector<glm::vec3> offsets;
    std::shared_ptr<Match::VertexBuffer> vertex_buffer;
    std::shared_ptr<Match::VertexBuffer> offset_buffer;
    std::shared_ptr<Match::IndexBuffer> index_buffer;
};
