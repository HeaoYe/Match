#include "application.hpp"
#include <imgui.h>

Application::Application() {
    Match::setting.device_name = Match::AUTO_SELECT_DEVICE;
    Match::setting.default_font_filename = "/usr/share/fonts/TTF/JetBrainsMonoNerdFontMono-Light.ttf";
    Match::setting.chinese_font_filename = "/usr/share/fonts/adobe-source-han-sans/SourceHanSansCN-Medium.otf";
    Match::setting.font_size = 24.0f;
    auto &context = Match::Initialize();

    // 启用了8xMSAA
    Match::runtime_setting->set_multisample_count(VK_SAMPLE_COUNT_8_BIT);
    Match::runtime_setting->set_vsync(true);
    
    auto factory = context.create_resource_factory("resource");
    auto builder = factory->create_render_pass_builder();
    // builder->add_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT, Match::AttachmentType::eColor);
    builder->add_attachment("depth", Match::AttachmentType::eDepth);
    auto &main_subpass = builder->add_subpass("main");
    main_subpass.attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
    main_subpass.attach_depth_attachment("depth");
    renderer = factory->create_renderer(builder);
    renderer->attach_render_layer<Match::ImGuiLayer>("imgui layer");

    scene_manager = std::make_unique<SceneManager>(factory, renderer);
}

Application::~Application() {
    renderer->wait_for_destroy();
    renderer.reset();
    scene_manager.reset();
    Match::Destroy();
}

void Application::gameloop() {
    while (Match::window->is_alive()) {
        Match::window->poll_events();

        scene_manager->update(ImGui::GetIO().DeltaTime);

        renderer->begin_render();
        scene_manager->render();

        renderer->begin_layer_render("imgui layer");
        scene_manager->render_imgui();
        renderer->end_layer_render("imgui layer");

        renderer->end_render();
    }
}
