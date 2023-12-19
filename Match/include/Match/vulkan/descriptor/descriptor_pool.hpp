#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class DescriptorPool {
        no_copy_move_construction(DescriptorPool)
    public:
        DescriptorPool();
        std::vector<VkDescriptorSet> allocate_descriptor_set(VkDescriptorSetLayout layout);
        ~DescriptorPool();
    INNER_VISIBLE:
        VkDescriptorPool descriptor_pool;
    };
}
