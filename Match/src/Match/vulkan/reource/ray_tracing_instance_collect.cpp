#include <Match/vulkan/resource/ray_tracing_instance_collect.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"

namespace Match {
    RayTracingInstanceCollect::RayTracingInstanceCollect(bool allow_update) : allow_update(allow_update) {
        register_custom_instance_info<InstanceAddressInfo>();
    }

    RayTracingInstanceCollect &RayTracingInstanceCollect::build() {
        auto &instance_address_infos = *static_cast<std::vector<InstanceAddressInfo> *>(instance_infos_buffer_map[get_class_hash_code<InstanceAddressInfo>()].stage_vector_address);
        auto instance_count = instance_address_infos.size();
        acceleration_struction_instance_infos_buffer = std::make_unique<Buffer>(instance_count * sizeof(vk::AccelerationStructureInstanceKHR), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        auto *acceleration_struction_instance_infos_ptr = static_cast<vk::AccelerationStructureInstanceKHR *>(acceleration_struction_instance_infos_buffer->map());

        uint32_t instance_index = 0;
        for (auto &[group_id, group_info] : groups) {
            group_info.instance_offset = instance_index;
            group_info.instance_count = group_info.instance_create_infos.size();
            for (auto &[model, transform_matrix] : group_info.instance_create_infos) {
                acceleration_struction_instance_infos_ptr
                    ->setInstanceCustomIndex(instance_index)
                    .setAccelerationStructureReference(get_acceleration_structure_address(model->acceleration_structure.value()->bottom_level_acceleration_structure))
                    .setInstanceShaderBindingTableRecordOffset(0)
                    .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                    .setMask(0xff)
                    .setTransform(transform<vk::TransformMatrixKHR, const glm::mat4 &>(transform_matrix));
                acceleration_struction_instance_infos_ptr ++;
                instance_index ++;
            }
            group_info.instance_create_infos.clear();
        }

        for (auto &[hash_code, buffer] : instance_infos_buffer_map) {
            buffer.create_buffer_callback();
        }

        vk::AccelerationStructureBuildRangeInfoKHR range {};
        range.setPrimitiveCount(instance_count)
            .setPrimitiveOffset(0)
            .setFirstVertex(0)
            .setTransformOffset(0);

        if (allow_update) {
            acceleration_struction_instance_infos_buffer->map();
        }

        vk::AccelerationStructureGeometryInstancesDataKHR instances {};
        instances.setData(get_buffer_address(acceleration_struction_instance_infos_buffer->buffer));
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
        
        auto size_info = manager->device->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build, instance_count, manager->dispatcher);
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
        }
        return *this;
    }

    RayTracingInstanceCollect &RayTracingInstanceCollect::update(uint32_t group, UpdateCallback update_callback) {
        multithread_update(group, [&](uint32_t batch_begin, uint32_t batch_end) {
            auto *acceleration_struction_instance_infos_ptr = static_cast<vk::AccelerationStructureInstanceKHR *>(acceleration_struction_instance_infos_buffer->map());
            for (uint32_t i = 0; i < batch_begin; i ++) {
                acceleration_struction_instance_infos_ptr ++;
            }
            InterfaceSetTransform interface_set_transform = [&](const glm::mat4 &transform_matrix) {
                acceleration_struction_instance_infos_ptr->setTransform(transform<vk::TransformMatrixKHR, const glm::mat4 &>(transform_matrix));
            };
            uint32_t index = 0;
            while (batch_begin < batch_end) {
                update_callback(index, interface_set_transform);
                acceleration_struction_instance_infos_ptr ++;
                index ++;
                batch_begin ++;
            }
        });

        auto &group_info = groups.at(group);

        vk::AccelerationStructureGeometryInstancesDataKHR instances {};
        instances.setData(get_buffer_address(acceleration_struction_instance_infos_buffer->buffer));
        vk::AccelerationStructureGeometryKHR geometry {};
        geometry.setGeometry(instances)
            .setGeometryType(vk::GeometryTypeKHR::eInstances);

        vk::AccelerationStructureBuildGeometryInfoKHR build {};
        build.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setMode(vk::BuildAccelerationStructureModeKHR::eUpdate)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
            .setGeometries(geometry);
        
        auto command_buffer = manager->command_pool->allocate_single_use();
        build.setSrcAccelerationStructure(instance_collect)
            .setDstAccelerationStructure(instance_collect)
            .setScratchData(get_buffer_address(scratch_buffer->buffer));
        vk::AccelerationStructureBuildRangeInfoKHR range {};
        range.setPrimitiveCount(group_info.instance_count)
            .setPrimitiveOffset(group_info.instance_offset)
            .setFirstVertex(0)
            .setTransformOffset(0);
        command_buffer.buildAccelerationStructuresKHR(build, &range, manager->dispatcher);
        manager->command_pool->free_single_use(command_buffer);

        return *this;
    }

    RayTracingInstanceCollect::~RayTracingInstanceCollect() {
        for (auto &[class_hash_code, buffer] : instance_infos_buffer_map) {
            buffer.release_callback();
            buffer.stage_vector_address = nullptr;
            buffer.buffer.reset();
        }
        instance_infos_buffer_map.clear();
    }

    uint32_t RayTracingInstanceCollect::add_instance_address_info(Model &model) {
        return add_instance_custom_info(InstanceAddressInfo {
            .vertex_buffer_address = get_buffer_address(model.acceleration_structure.value()->vertex_buffer->buffer),
            .index_buffer_address = get_buffer_address(model.acceleration_structure.value()->index_buffer->buffer),
        });
    }

    bool RayTracingInstanceCollect::check_model_suitable(Model &model) {
        if (model.acceleration_structure.has_value()) {
            return true;
        }
        MCH_WARN("Can't add the model which doesn't have acceleration structure")
        return false;
    }
    
    void RayTracingInstanceCollect::multithread_update(uint32_t group, UpdateBatchCallback update_batch_callback) {
        if (!allow_update) {
            MCH_WARN("This RayTracingInstanceCollect doesn't allow update")
            return ;
        }

        auto &group_info = groups.at(group);
        uint32_t begin = group_info.instance_offset;
        uint32_t end = begin + group_info.instance_count;

        std::vector<std::thread> thread_pool;
        while (begin < end) {
            uint32_t batch_end = std::min(begin + 500, end);
            thread_pool.emplace_back([&]() {
                update_batch_callback(begin, batch_end);
            });
            begin = batch_end;
        }
        for (auto &thread : thread_pool) {
            thread.join();
        }
    }
}
