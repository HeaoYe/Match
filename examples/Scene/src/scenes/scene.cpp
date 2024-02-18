#include "scene.hpp"
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

Scene::Scene(std::shared_ptr<Match::ResourceFactory> factory) : factory(factory) {}

Scene::~Scene() {
}

void SceneManager::update() {
    if (current_scene.get() == nullptr) {
        return;
    }
    current_scene->update(ImGui::GetIO().DeltaTime);
}

void SceneManager::render() {
    if (current_scene.get() == nullptr) {
        return;
    }
    // begin_render() 会干两件事
    // 1. acquire_next_image();  获取下一帧的图像用于渲染
    // 2. begin_render_pass();   开启RenderPass
    // 对于光追场景,需要在current_scene->render()中手动开启RenderPass
    current_scene->renderer->acquire_next_image();
    if (!current_scene->is_ray_tracing_scene) {
        current_scene->renderer->begin_render_pass();
    }
    current_scene->render();

    current_scene->renderer->begin_layer_render("imgui layer");
    current_scene->render_imgui();
    ImGui::SeparatorText("Scene Manager");
    for (auto &[name, callback] : load_scene_callbacks) {
        if (ImGui::Button(name.c_str())) {
            current_callback = callback;
        }
    }
    current_scene->renderer->end_layer_render("imgui layer");

    current_scene->renderer->end_render();

    if (current_callback.has_value()) {
        current_callback.value()();
        current_callback.reset();
    }
}

void SceneManager::destroy_scene() {
    if (current_scene.get() != nullptr) {
        current_scene->renderer->wait_for_destroy();
        current_scene->destroy();
        current_scene.reset();
    }
}

SceneManager::~SceneManager() {
    destroy_scene();
    factory.reset();
}
