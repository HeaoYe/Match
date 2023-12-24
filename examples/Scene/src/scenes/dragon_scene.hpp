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
        alignas(4) uint num;
    } data;
    LightController(std::weak_ptr<Match::ResourceFactory> factory);
    void update();
    std::shared_ptr<Match::UniformBuffer> uniform;
};

// 使用宏定义Scene
class DragonScene final : public Scene {
    define_scene(DragonScene)
private:
    // 场景资源
    std::unique_ptr<Camera> camera;
    std::unique_ptr<LightController> light;
    std::shared_ptr<Match::ShaderProgram> shader_program;
    std::vector<glm::vec3> offsets;
    std::shared_ptr<Match::VertexBuffer> vertex_buffer;
    std::shared_ptr<Match::VertexBuffer> offset_buffer;
    std::shared_ptr<Match::IndexBuffer> index_buffer;
};
