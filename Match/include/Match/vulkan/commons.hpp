#pragma once

#include <Match/commons.hpp>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#define __vk_test(result, LEVEL) if ((result) != VK_SUCCESS) { MCH_##LEVEL("File: {} Line: {} Func: {} Vulkan Check Faild", __FILE__, __LINE__, __FUNCTION__)};
#define vk_check(result) __vk_test(result, ERROR)
#define vk_assert(result) __vk_test(result, FATAL)

namespace Match {
}
