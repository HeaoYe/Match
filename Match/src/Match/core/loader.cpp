#include <Match/core/loader.hpp>
#include <Match/vulkan/manager.hpp>

namespace Match {
    APIInfo &CreateAPIInfo() {
        return APIManager::CreateAPIInfo();
    }

    std::vector<std::string> EnumerateDevices() {
        return APIManager::GetInstance().enumerate_devices();
    }

    APIManager &Initialize() {
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
    }
}
