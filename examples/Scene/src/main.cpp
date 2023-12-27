#include "application.hpp"
#include "scenes/dragon_scene.hpp"
#include "scenes/shadertoy_scene.hpp"

int main() {
    // 窗口大小的配置移动到main.cpp中
    Match::setting.window_size = { 1920, 1080 };
    // 关闭debug_moed和MSAA可以提高一点性能
    Match::setting.debug_mode = true;
    Application app;
    // 中国龙场景
    // app.load_scene<DragonScene>();
    // ShaderToy 简单的支持
    // 支持所有只有一个Channel的Shader
    app.load_scene<ShaderToyScene>();
    app.gameloop();
    return 0;
}
