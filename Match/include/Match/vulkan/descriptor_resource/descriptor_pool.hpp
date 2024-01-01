#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class DescriptorPool {
        no_copy_move_construction(DescriptorPool)
    public:
        DescriptorPool();
        std::vector<vk::DescriptorSet> allocate_descriptor_set(vk::DescriptorSetLayout layout);
        void free_descriptor_set(vk::DescriptorSet set);
        ~DescriptorPool();
    INNER_VISIBLE:
        vk::DescriptorPool descriptor_pool;
    };
}
