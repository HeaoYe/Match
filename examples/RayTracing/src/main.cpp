#include <Match/Match.hpp>

int main() {
    Match::setting.debug_mode = true;
    Match::setting.enable_ray_tracing = true;
    auto &ctx = Match::Initialize();
    
    while (Match::window->is_alive()) {
        Match::window->poll_events();
    }

    Match::Destroy();
}
