#pragma once

#include <Match/Match.hpp>
#include <glm/glm.hpp>
#include <imgui.h>

// Vertex类型集成到Match渲染引擎中了

// 添加了定义Scene的宏
#define define_scene(scene_cls) \
public: \
    scene_cls(std::shared_ptr<Match::ResourceFactory> factory) : Scene(factory) {} \
    void initialize() override; \
    void update(float delta) override; \
    void render() override; \
    void render_imgui() override; \
    void destroy() override;

class SceneManager;

class Scene {
    friend SceneManager;
public:
    Scene(std::shared_ptr<Match::ResourceFactory> factory);
    virtual void initialize() = 0;
    virtual void update(float delta) = 0;
    virtual void render() = 0;
    virtual void render_imgui() = 0;
    virtual void destroy() = 0;
    virtual ~Scene();
protected:
    // 模型加载功能也集成了
    std::shared_ptr<Match::ResourceFactory> factory;
    std::shared_ptr<Match::Renderer> renderer;
};

class SceneManager {
    using LoadSceneCallback = std::function<void()>;
public:
    SceneManager(std::shared_ptr<Match::ResourceFactory> factory) : factory(factory) {}
    ~SceneManager();

    template <class SceneClass>
    void register_scene(const std::string &name) {
        load_scene_callbacks.insert(std::make_pair(name, [this]() {
            destroy_scene();
            current_scene = std::make_unique<SceneClass>(factory);
            current_scene->initialize();
        }));
        if (current_scene.get() == nullptr) {
            load_scene_callbacks.begin()->second();
        }
    }

    bool has_scene() { return current_scene.get() != nullptr; }
    void update();
    void render();
private:
    void destroy_scene();
private:
    std::shared_ptr<Match::ResourceFactory> factory;
    std::unique_ptr<Scene> current_scene;
    std::map<std::string, LoadSceneCallback> load_scene_callbacks;
    std::optional<LoadSceneCallback> current_callback;
};
