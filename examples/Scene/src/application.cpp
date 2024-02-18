#include "application.hpp"
#include <imgui.h>

Application::Application() {
    Match::setting.device_name = Match::AUTO_SELECT_DEVICE;
    Match::setting.chinese_font_filename = "../fonts/SourceHanSansCN-Normal.ttf";
#if !defined(PLATFORM_WINDOWS)
    Match::setting.default_font_filename = "/usr/share/fonts/TTF/JetBrainsMonoNerdFontMono-Light.ttf";
#endif
    Match::setting.font_size = 24.0f;
    auto &context = Match::Initialize();

    // 启用了8xMSAA
    Match::runtime_setting->set_multisample_count(Match::SampleCount::e8);
    Match::runtime_setting->set_vsync(true);
    
    auto factory = context.create_resource_factory("resource");
    scene_manager = std::make_unique<SceneManager>(factory);
}

Application::~Application() {
    scene_manager.reset();
    Match::Destroy();
}

void Application::gameloop() {
    while (Match::window->is_alive()) {
        Match::window->poll_events();

        scene_manager->update();

        scene_manager->render();
    }
}
