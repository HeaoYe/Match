#include <Match/Match.hpp>
#include "scene.hpp"

class Application {
public:
    Application();
    void gameloop();
    ~Application();
private:
    std::shared_ptr<Match::ResourceFactory> factory;
    std::shared_ptr<Match::Renderer> renderer;
    std::unique_ptr<Scene> scene;
};
