#include <Match/core/loader.hpp>
#include <Match/core/window.hpp>
#include <Match/vulkan/manager.hpp>

namespace Match {
    std::vector<std::string> EnumerateDevices() {
        return APIManager::GetInstance().enumerate_devices();
    }

    APIManager &Initialize() {
        glfwInit();
        window = std::make_unique<Window>();
        g_logger.initialize();
        runtime_setting = APIManager::GetInstance().get_runtime_setting();
        APIManager::GetInstance().initialize();
        return APIManager::GetInstance();
    }

    void Destroy() {
        APIManager::GetInstance().destroy();
        runtime_setting.reset();
        APIManager::Quit();
        g_logger.destroy();
        window.reset();
        glfwTerminate();
    }
}
