#include <Match/vulkan/descriptor_resource/descriptor_pool.hpp>
#include "../inner.hpp"

namespace Match {
    DescriptorPool::DescriptorPool() {
        std::vector<vk::DescriptorPoolSize> pool_sizes = {};
        vk::DescriptorPoolCreateInfo pool_create_info {};
        pool_create_info.setPoolSizes(pool_sizes)
            .setMaxSets(16)
            .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
        descriptor_pool = manager->device->device.createDescriptorPool(pool_create_info);
    }

    std::vector<vk::DescriptorSet> DescriptorPool::allocate_descriptor_set(vk::DescriptorSetLayout layout) {
        std::vector<vk::DescriptorSet> descriptor_sets(setting.max_in_flight_frame);
        std::vector<vk::DescriptorSetLayout> layouts(setting.max_in_flight_frame, layout);
        vk::DescriptorSetAllocateInfo alloc_info {};
        alloc_info.setDescriptorPool(descriptor_pool)
            .setDescriptorSetCount(static_cast<uint32_t>(setting.max_in_flight_frame))
            .setSetLayouts(layouts);
        descriptor_sets = manager->device->device.allocateDescriptorSets(alloc_info);
        return descriptor_sets;
    }

    void DescriptorPool::free_descriptor_set(vk::DescriptorSet descriptor_set) {
        manager->device->device.freeDescriptorSets(descriptor_pool, descriptor_set);
    }

    DescriptorPool::~DescriptorPool(){
        manager->device->device.destroyDescriptorPool(descriptor_pool);
    }
}
