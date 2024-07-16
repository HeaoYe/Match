#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class DescriptorPool {
        no_copy_move_construction(DescriptorPool)
    public:
        MATCH_API DescriptorPool();
        MATCH_API std::vector<vk::DescriptorSet> allocate_descriptor_sets(vk::DescriptorSetLayout layout);
        MATCH_API void free_descriptor_sets(const std::vector<vk::DescriptorSet> &sets);
        MATCH_API ~DescriptorPool();
    INNER_VISIBLE:
        vk::DescriptorPool descriptor_pool;
    };
}
