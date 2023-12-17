#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class CommandPool {
        no_copy_move_construction(CommandPool)
    public:
        CommandPool(VkCommandPoolCreateFlags flags);
        ~CommandPool();
        void allocate_command_buffer(VkCommandBuffer *command_buffers, uint32_t count);
    INNER_VISIBLE:
        VkCommandPool command_pool;
    };
}
