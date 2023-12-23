#include <Match/Match.hpp>

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 in_pos;
    glm::vec3 in_normal;
    glm::vec3 in_color;
 
    bool operator==(const Vertex& other) const {
        return in_pos == other.in_pos && in_normal == other.in_normal && in_color == other.in_color;
    }
};

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

class Camera {
public:
    struct CameraUniform {
        alignas(4) glm::vec3 pos { 0, 0, 0 };
        alignas(4) glm::vec3 direction { 0, 0, 1 };
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 project;
    } data;
    float yaw, pitch;
    Camera(std::weak_ptr<Match::ResourceFactory> factory);
    void update();
    std::shared_ptr<Match::UniformBuffer> uniform;
    bool flag = true;
};

class Scene {
public:
    Scene(std::shared_ptr<Match::ResourceFactory> factory, std::shared_ptr<Match::Renderer> renderer);
    void update(float delta);
    void render();
    void render_imgui();
private:
    void load_model(const std::string &filename);
private:
    std::shared_ptr<Match::ResourceFactory> factory;
    std::shared_ptr<Match::Renderer> renderer;
private:
    // 场景资源
    std::unique_ptr<Camera> camera;
    std::unique_ptr<LightController> light;
    std::shared_ptr<Match::ShaderProgram> shader_program;
    std::vector<Vertex> vertices;
    std::vector<glm::vec3> offsets;
    std::vector<uint32_t> indices;
    std::shared_ptr<Match::VertexBuffer> vertex_buffer;
    std::shared_ptr<Match::VertexBuffer> offset_buffer;
    std::shared_ptr<Match::IndexBuffer> index_buffer;
};
