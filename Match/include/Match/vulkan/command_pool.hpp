#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class CommandPool {
        no_copy_move_construction(CommandPool)
    public:
        MATCH_API CommandPool(vk::CommandPoolCreateFlags flags);
        MATCH_API ~CommandPool();
        MATCH_API std::vector<vk::CommandBuffer> allocate_command_buffer(uint32_t count);
        MATCH_API vk::CommandBuffer allocate_single_use();
        MATCH_API void free_single_use(vk::CommandBuffer command_buffer);
    INNER_VISIBLE:
        vk::CommandPool command_pool;
    };
}
