#include <Match/vulkan/command_pool.hpp>
#include "inner.hpp"

namespace Match {
    CommandPool::CommandPool(VkCommandPoolCreateFlags flags) {
        VkCommandPoolCreateInfo command_pool_create_info { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        command_pool_create_info.flags = flags;
        command_pool_create_info.queueFamilyIndex = manager->device->graphics_family_index;
        vk_check(vkCreateCommandPool(manager->device->device, &command_pool_create_info, manager->allocator, &command_pool));
    }

    CommandPool::~CommandPool() {
        vkDestroyCommandPool(manager->device->device, command_pool, manager->allocator);
    }

    void CommandPool::allocate_command_buffer(VkCommandBuffer *command_buffers, uint32_t count) {
        VkCommandBufferAllocateInfo allocate_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocate_info.commandPool = command_pool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = count;
        vk_check(vkAllocateCommandBuffers(manager->device->device, &allocate_info, command_buffers));
    }

    VkCommandBuffer CommandPool::allocate_single_use() {
        VkCommandBuffer command_buffer;
        VkCommandBufferAllocateInfo command_buffer_allocate_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        command_buffer_allocate_info.commandPool = command_pool;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = 1;
        vk_check(vkAllocateCommandBuffers(manager->device->device, &command_buffer_allocate_info, &command_buffer))

        VkCommandBufferBeginInfo begin_info { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vk_check(vkBeginCommandBuffer(command_buffer, &begin_info))

        return command_buffer;
    }

    void CommandPool::free_single_use(VkCommandBuffer command_buffer) {
        vkEndCommandBuffer(command_buffer);
        VkSubmitInfo submit_info { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.pCommandBuffers = &command_buffer;
        submit_info.commandBufferCount = 1;
        vk_check(vkQueueSubmit(manager->device->transfer_queue, 1, &submit_info, VK_NULL_HANDLE))
        vk_check(vkQueueWaitIdle(manager->device->transfer_queue))

        vkFreeCommandBuffers(manager->device->device, command_pool, 1, &command_buffer);
    }
}
