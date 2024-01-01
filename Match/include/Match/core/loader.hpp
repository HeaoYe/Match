#pragma once

#include <Match/commons.hpp>
#include <Match/vulkan/manager.hpp>
#include <Match/vulkan/resource/resource_factory.hpp>
#include <Match/vulkan/renderpass.hpp>

namespace Match {
    std::vector<std::string> EnumerateDevices();
    APIManager &Initialize();
    void Destroy();
}
