#include "application.hpp"
#include "scenes/dragon_scene.hpp"
#include "scenes/shadertoy_scene.hpp"
#include "scenes/pbr_scene.hpp"
#include "scenes/ssao_scene.hpp"
#include "scenes/raymarching_scene.hpp"
#include "scenes/model_viewer_scene.hpp"
#include "scenes/ray_tracing_scene.hpp"
#include "scenes/ray_tracing_v2_scene.hpp"
#include "scenes/pbr_path_tracing_scene.hpp"

int main() {
    // 窗口大小的配置移动到main.cpp中
    Match::setting.window_size = { 1920, 1080 };
    // 关闭debug_moed和MSAA可以提高一点性能
    Match::setting.debug_mode = true;
    // 开启Match的光追功能
    Match::setting.enable_ray_tracing = true;

    Application app;
    // 第一个注册的Scene为默认加载的Scene

    // 蒙特卡洛PBR路径追踪场景
    app.register_scene<PBRPathTracingScene>("蒙特卡洛PBR路径追踪");
    // 简易光线追踪2.0场景
    app.register_scene<RayTracingV2Scene>("简易光线追踪2.0");
    // 简易光线追踪场景
    app.register_scene<RayTracingScene>("简易光线追踪");
    // 模型加载器场景
    app.register_scene<ModelViewerScene>("模型加载器");
    // 光线步进场景
    app.register_scene<RayMarchingScene>("光线步进");
    // 环境光遮蔽场景
    app.register_scene<SSAOScene>("环境光遮蔽");
    // Physically Based Rendering
    app.register_scene<PBRScene>("Physically Based Rendering");
    // ShaderToy 简单的支持
    // 支持所有只有一个Channel的Shader
    app.register_scene<ShaderToyScene>("Shader Toy");
    // 中国龙场景
    app.register_scene<DragonScene>("中国龙");
    app.gameloop();
    return 0;
}
