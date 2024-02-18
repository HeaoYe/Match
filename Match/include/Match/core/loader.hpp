#pragma once

#include <Match/commons.hpp>
#include <Match/vulkan/manager.hpp>
#include <Match/vulkan/resource/resource_factory.hpp>
#include <Match/vulkan/renderpass.hpp>

namespace Match {
    MATCH_API std::vector<std::string> EnumerateDevices();
    MATCH_API APIManager &Initialize();
    MATCH_API void Destroy();
}
