#pragma once

#include <Match/Match.hpp>
#include "scenes/scene.hpp"

class Application {
public:
    Application();
    template <class SceneClass>
    void register_scene(const std::string &name) {
        scene_manager->register_scene<SceneClass>(name);
    }
    void gameloop();
    ~Application();
private:
    std::unique_ptr<SceneManager> scene_manager;
};
