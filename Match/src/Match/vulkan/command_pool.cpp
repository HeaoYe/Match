#include <Match/vulkan/command_pool.hpp>
#include "inner.hpp"

namespace Match {
    CommandPool::CommandPool(VkCommandPoolCreateFlags flags) {
        VkCommandPoolCreateInfo command_pool_create_info { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        command_pool_create_info.flags = flags;
        command_pool_create_info.queueFamilyIndex = manager->device->graphics_family_index;
        vk_check(vkCreateCommandPool(manager->device->device, &command_pool_create_info, manager->alloctor, &command_pool));
    }

    CommandPool::~CommandPool() {
        vkDestroyCommandPool(manager->device->device, command_pool, manager->alloctor);
    }

    void CommandPool::allocate_command_buffer(VkCommandBuffer *command_buffers, uint32_t count) {
        VkCommandBufferAllocateInfo allocate_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocate_info.commandPool = command_pool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = count;
        vk_check(vkAllocateCommandBuffers(manager->device->device, &allocate_info, command_buffers));
    }
}
