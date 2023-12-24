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

class DragonScene final : public Scene {
public:
    DragonScene(std::shared_ptr<Match::ResourceFactory> factory, std::shared_ptr<Match::Renderer> renderer) : Scene(factory, renderer) {}
    void initialize() override;
    void update(float delta) override;
    void render() override;
    void render_imgui() override;
    void destroy() override;
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
