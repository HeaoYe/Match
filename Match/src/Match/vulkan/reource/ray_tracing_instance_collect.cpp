#include <Match/vulkan/resource/ray_tracing_instance_collect.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"

namespace Match {
    RayTracingInstanceCollectBase::RayTracingInstanceCollectBase(bool allow_update) : allow_update(allow_update) {}

    RayTracingInstanceCollectBase::~RayTracingInstanceCollectBase() {
        manager->device->device.destroyAccelerationStructureKHR(instance_collect, nullptr, manager->dispatcher);
        instance_collect_buffer.reset();
        scratch_buffer.reset();
        acceleration_struction_instance_infos_buffer.reset();
        acceleration_struction_instance_infos.clear();
        instance_infos_buffer.reset();
    }

    void RayTracingInstanceCollectBase::build_acceleration_structure() {
        prepare_instance_infos();

        vk::AccelerationStructureBuildRangeInfoKHR range {};
        range.setPrimitiveCount(acceleration_struction_instance_infos.size())
            .setPrimitiveOffset(0)
            .setFirstVertex(0)
            .setTransformOffset(0);

        acceleration_struction_instance_infos_buffer = std::make_unique<TwoStageBuffer>(acceleration_struction_instance_infos.size() * sizeof(vk::AccelerationStructureInstanceKHR), vk::BufferUsageFlags {}, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);
        if (allow_update) {
            acceleration_struction_instance_infos_buffer->map();
            instance_infos_buffer->map();
        }
        acceleration_struction_instance_infos_buffer->upload_data_from_vector(acceleration_struction_instance_infos);

        vk::AccelerationStructureGeometryInstancesDataKHR instances {};
        instances.setData(get_buffer_address(acceleration_struction_instance_infos_buffer->buffer->buffer));
        vk::AccelerationStructureGeometryKHR geometry {};
        geometry.setGeometry(instances)
            .setGeometryType(vk::GeometryTypeKHR::eInstances);

        vk::AccelerationStructureBuildGeometryInfoKHR build {};
        vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
        if (allow_update) {
            flags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
        }
        build.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setFlags(flags)
            .setGeometries(geometry);
        
        auto size_info = manager->device->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build, acceleration_struction_instance_infos.size(), manager->dispatcher);
        scratch_buffer = std::make_unique<Buffer>(size_info.buildScratchSize, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        instance_collect_buffer = std::make_shared<Buffer>(size_info.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        
        vk::AccelerationStructureCreateInfoKHR instance_create_info {};
        instance_create_info.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setSize(size_info.accelerationStructureSize)
            .setBuffer(instance_collect_buffer->buffer);
        instance_collect = manager->device->device.createAccelerationStructureKHR(instance_create_info, nullptr, manager->dispatcher);

        auto command_buffer = manager->command_pool->allocate_single_use();
        build.setDstAccelerationStructure(instance_collect)
            .setScratchData(get_buffer_address(scratch_buffer->buffer));
        command_buffer.buildAccelerationStructuresKHR(build, &range, manager->dispatcher);
        manager->command_pool->free_single_use(command_buffer);

        if (!allow_update) {
            scratch_buffer.reset();
            acceleration_struction_instance_infos_buffer.reset();
            acceleration_struction_instance_infos.clear();
        }
    }

    void RayTracingInstanceCollectBase::update_acceleration_structure(uint32_t offset, uint32_t count, update_instance_infos_func update_instance_infos) {
        uint32_t batch_begin = offset;
        uint32_t end = batch_begin + count;
        std::vector<std::thread> thread_pool;
        while (batch_begin < end) {
            uint32_t batch_end = std::min(batch_begin + 500, end);
            thread_pool.emplace_back([&]() {
                update_instance_infos(batch_begin, batch_end);
            });
            thread_pool.back().join();
            batch_begin = batch_end;
        }
        for (auto &thread : thread_pool) {
            // thread.join();
        }
        acceleration_struction_instance_infos_buffer->flush();

        vk::AccelerationStructureGeometryInstancesDataKHR instances {};
        instances.setData(get_buffer_address(acceleration_struction_instance_infos_buffer->buffer->buffer));
        vk::AccelerationStructureGeometryKHR geometry {};
        geometry.setGeometry(instances)
            .setGeometryType(vk::GeometryTypeKHR::eInstances);

        vk::AccelerationStructureBuildGeometryInfoKHR build {};
        build.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setMode(vk::BuildAccelerationStructureModeKHR::eUpdate)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
            .setGeometries(geometry);
        
        auto size_info = manager->device->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build, acceleration_struction_instance_infos.size(), manager->dispatcher);
        if (size_info.buildScratchSize > scratch_buffer->size) {
            scratch_buffer.reset();
            scratch_buffer = std::make_unique<Buffer>(size_info.buildScratchSize, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        }
        
        auto command_buffer = manager->command_pool->allocate_single_use();
        build.setSrcAccelerationStructure(instance_collect)
            .setDstAccelerationStructure(instance_collect)
            .setScratchData(get_buffer_address(scratch_buffer->buffer));
        vk::AccelerationStructureBuildRangeInfoKHR range {};
        range.setPrimitiveCount(count)
            .setPrimitiveOffset(offset)
            .setFirstVertex(0)
            .setTransformOffset(0);
        command_buffer.buildAccelerationStructuresKHR(build, &range, manager->dispatcher);
        manager->command_pool->free_single_use(command_buffer);
    }

    bool RayTracingInstanceCollectBase::check_model(Model &model) {
        if (!model.acceleration_structure.has_value()) {
            MCH_ERROR("Model doesn't have acceleration structure")
            return false;
        }
        return true;
    }

    std::tuple<vk::DeviceAddress, vk::DeviceAddress, vk::DeviceAddress> RayTracingInstanceCollectBase::get_model_addresses(Model &model) {
        return {
            get_acceleration_structure_address(model.acceleration_structure.value()->bottom_level_acceleration_structure),
            get_buffer_address(model.acceleration_structure.value()->vertex_buffer->buffer),
            get_buffer_address(model.acceleration_structure.value()->index_buffer->buffer),
        };
    }
}
