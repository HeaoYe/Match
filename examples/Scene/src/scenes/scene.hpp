#pragma once

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

class Scene {
public:
    Scene(std::shared_ptr<Match::ResourceFactory> factory, std::shared_ptr<Match::Renderer> renderer);
    virtual void initialize() = 0;
    virtual void update(float delta) = 0;
    virtual void render() = 0;
    virtual void render_imgui() = 0;
    virtual void destroy() = 0;
    virtual ~Scene();
protected:
    void load_model(const std::string &filename);
    std::shared_ptr<Match::ResourceFactory> factory;
    std::shared_ptr<Match::Renderer> renderer;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

class SceneManager {
public:
    SceneManager(std::shared_ptr<Match::ResourceFactory> factory, std::shared_ptr<Match::Renderer> renderer) : factory(factory), renderer(renderer) {}
    ~SceneManager();
    template <class SceneClass>
    void load_scene() {
        if (current_scene.get() != nullptr) {
            current_scene->destroy();
            current_scene.reset();
        }
        current_scene = std::make_unique<SceneClass>(factory, renderer);
        current_scene->initialize();
    }

    void update(float delta);
    void render();
    void render_imgui();
private:
    std::shared_ptr<Match::ResourceFactory> factory;
    std::shared_ptr<Match::Renderer> renderer;
    std::unique_ptr<Scene> current_scene;
};
