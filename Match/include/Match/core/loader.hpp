#pragma once

#include <Match/commons.hpp>
#include <Match/vulkan/api_info.hpp>
#include <Match/vulkan/resource/resource_factory.hpp>
#include <Match/vulkan/renderpass.hpp>

namespace Match {
    APIInfo CreateAPIInfo();
    std::vector<std::string> EnumerateDevices();
    APIManager &Initialize(const APIInfo &info = {});
    void Destroy();
}
