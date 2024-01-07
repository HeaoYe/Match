#include <Match/vulkan/resource/ray_tracing_instance.hpp>
#include <Match/vulkan/utils.hpp>
#include "../inner.hpp"

namespace Match {
    RayTracingInstance::RayTracingInstance(const std::vector<std::shared_ptr<Model>> &models) {
        std::vector<vk::AccelerationStructureInstanceKHR> model_infos;
        std::vector<InstanceAddress> instance_address_infos;
        for (auto &model : models) {
            if (!model->acceleration_structure.has_value()) {
                MCH_ERROR("Model doesn't have acceleration structure")
                continue;
            }
            auto &acceleration_structure = model->acceleration_structure.value();
            model_infos.emplace_back()
                .setInstanceCustomIndex(instance_address_infos.size())
                .setInstanceShaderBindingTableRecordOffset(0)
                .setAccelerationStructureReference(get_acceleration_structure_address(acceleration_structure->bottom_level_acceleration_structure))
                .setMask(0xff)
                .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                .setTransform(std::array {
                    std::array<float, 4> { 1, 0, 0, 0 },
                    std::array<float, 4> { 0, 1, 0, 0 },
                    std::array<float, 4> { 0, 0, 1, 0 },
                });
            instance_address_infos.push_back({
                .vertex_buffer_address = get_buffer_address(acceleration_structure->vertex_buffer->buffer),
                .index_buffer_address = get_buffer_address(acceleration_structure->index_buffer->buffer),
            });
        }
        vk::AccelerationStructureBuildRangeInfoKHR range {};
        range.setPrimitiveCount(models.size())
            .setPrimitiveOffset(0)
            .setFirstVertex(0)
            .setTransformOffset(0);
        TwoStageBuffer model_infos_buffer (model_infos.size() * sizeof(vk::AccelerationStructureInstanceKHR), vk::BufferUsageFlags {}, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);
        instance_addresses = std::make_shared<TwoStageBuffer>(instance_address_infos.size() * sizeof(InstanceAddress), vk::BufferUsageFlagBits::eStorageBuffer);
        model_infos_buffer.upload_data_from_vector(model_infos);
        instance_addresses->upload_data_from_vector(instance_address_infos);


        vk::AccelerationStructureGeometryInstancesDataKHR instances {};
        instances.setData(get_buffer_address(model_infos_buffer.buffer->buffer));
        vk::AccelerationStructureGeometryKHR geometry {};
        geometry.setGeometry(instances)
            .setGeometryType(vk::GeometryTypeKHR::eInstances);

        vk::AccelerationStructureBuildGeometryInfoKHR build {};
        build.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
            .setGeometries(geometry);
        
        auto size_info = manager->device->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build, model_infos.size(), manager->dispatcher);
        instance_buffer = std::make_shared<Match::Buffer>(size_info.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        Match::Buffer scratch_buffer(size_info.buildScratchSize, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        
        vk::AccelerationStructureCreateInfoKHR instance_create_info {};
        instance_create_info.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setSize(size_info.accelerationStructureSize)
            .setBuffer(instance_buffer->buffer);
        instance = manager->device->device.createAccelerationStructureKHR(instance_create_info, nullptr, manager->dispatcher);

        auto command_buffer = manager->command_pool->allocate_single_use();
        build.setDstAccelerationStructure(instance)
            .setScratchData(get_buffer_address(scratch_buffer.buffer));
        command_buffer.buildAccelerationStructuresKHR(build, &range, manager->dispatcher);
        manager->command_pool->free_single_use(command_buffer);
    }

    RayTracingInstance::~RayTracingInstance() {
        manager->device->device.destroyAccelerationStructureKHR(instance, nullptr, manager->dispatcher);
        instance_buffer.reset();
        instance_addresses.reset();
    }
}
