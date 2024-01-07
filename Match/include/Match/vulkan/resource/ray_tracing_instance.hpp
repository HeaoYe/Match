#pragma once

#include <Match/vulkan/resource/model.hpp>

namespace Match {
    class RayTracingInstance {
        no_copy_move_construction(RayTracingInstance)
        struct InstanceAddress {
            vk::DeviceAddress vertex_buffer_address;
            vk::DeviceAddress index_buffer_address;
        };
    public:
        RayTracingInstance(const std::vector<std::shared_ptr<Model>> &models);
        std::shared_ptr<TwoStageBuffer> get_instance_addresses_bufer() { return instance_addresses; }
        ~RayTracingInstance();
    INNER_VISIBLE:
        vk::AccelerationStructureKHR instance;
        std::shared_ptr<Buffer> instance_buffer;
        std::shared_ptr<TwoStageBuffer> instance_addresses;
    };
}
