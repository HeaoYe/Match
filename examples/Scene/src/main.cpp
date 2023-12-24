#include "application.hpp"
#include "scenes/dragon_scene.hpp"

int main() {
    Application app;
    app.load_scene<DragonScene>();
    app.gameloop();
    return 0;
}
