#pragma once

#include <Match/vulkan/resource/model.hpp>
#include <glm/glm.hpp>

namespace Match {
    class RayTracingInstance {
        default_no_copy_move_construction(RayTracingInstance)
        struct InstanceAddress {
            vk::DeviceAddress vertex_buffer_address;
            vk::DeviceAddress index_buffer_address;
        };
        using interface_update_transform_func = std::function<void(const glm::mat4 &)>;
        using update_func = std::function<void(uint32_t, interface_update_transform_func interface_update_transform)>;
    public:
        RayTracingInstance &add_model(std::shared_ptr<Model> model, const glm::mat4 &matrix = {
            { 1, 0, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0, 1, 0 },
            { 0, 0, 0, 1 },
        });
        RayTracingInstance &build();
        RayTracingInstance &update(update_func func);
        std::shared_ptr<TwoStageBuffer> get_instance_addresses_bufer() { return instance_addresses; }
        ~RayTracingInstance();
    INNER_VISIBLE:
        vk::AccelerationStructureKHR instance;
        std::shared_ptr<Buffer> instance_buffer;
        std::shared_ptr<TwoStageBuffer> instance_addresses;
        std::vector<std::pair<std::shared_ptr<Model>, glm::mat4>> model_infos;
        std::vector<vk::AccelerationStructureInstanceKHR> instance_infos;
    };
}
