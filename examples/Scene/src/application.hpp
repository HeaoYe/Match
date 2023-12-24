#include <Match/Match.hpp>
#include "scenes/scene.hpp"

class Application {
public:
    Application();
    template <class SceneClass>
    void load_scene() {
        scene_manager->load_scene<SceneClass>();
    }
    void gameloop();
    ~Application();
private:
    std::shared_ptr<Match::Renderer> renderer;
    std::unique_ptr<SceneManager> scene_manager;
};
