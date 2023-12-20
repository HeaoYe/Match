#include <Match/vulkan/descriptor/descriptor_pool.hpp>
#include "../inner.hpp"

namespace Match {
    DescriptorPool::DescriptorPool() {
        std::vector<VkDescriptorPoolSize> pool_sizes = {};
        VkDescriptorPoolCreateInfo pool_create_info { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        pool_create_info.poolSizeCount = pool_sizes.size();
        pool_create_info.pPoolSizes = pool_sizes.data();
        pool_create_info.maxSets = 16;
        vkCreateDescriptorPool(manager->device->device, &pool_create_info, manager->allocator, &descriptor_pool);
    }

    std::vector<VkDescriptorSet> DescriptorPool::allocate_descriptor_set(VkDescriptorSetLayout layout) {
        std::vector<VkDescriptorSet> descriptor_sets(setting.max_in_flight_frame);
        std::vector<VkDescriptorSetLayout> layouts(setting.max_in_flight_frame, layout);
        VkDescriptorSetAllocateInfo alloc_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        alloc_info.descriptorPool = manager->descriptor_pool->descriptor_pool;
        alloc_info.descriptorSetCount = static_cast<uint32_t>(setting.max_in_flight_frame);
        alloc_info.pSetLayouts = layouts.data();
        vkAllocateDescriptorSets(manager->device->device, &alloc_info, descriptor_sets.data());
        return descriptor_sets;
    }

    DescriptorPool::~DescriptorPool(){
        vkDestroyDescriptorPool(manager->device->device, descriptor_pool, manager->allocator);
    }
}
