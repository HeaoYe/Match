#include <Match/vulkan/resource/ray_tracing_instance.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"

namespace Match {
    RayTracingInstance &RayTracingInstance::add_model(std::shared_ptr<Model> model, const glm::mat4 &matrix) {
        model_infos.push_back(std::make_pair(model, matrix));
        return *this;
    }

    RayTracingInstance &RayTracingInstance::build() {
        std::vector<InstanceAddress> instance_address_infos;
        for (auto &[model, matrix] : model_infos) {
            if (!model->acceleration_structure.has_value()) {
                MCH_ERROR("Model doesn't have acceleration structure")
                continue;
            }
            auto &acceleration_structure = model->acceleration_structure.value();
            instance_infos.emplace_back()
                .setInstanceCustomIndex(instance_address_infos.size())
                .setInstanceShaderBindingTableRecordOffset(0)
                .setAccelerationStructureReference(get_acceleration_structure_address(acceleration_structure->bottom_level_acceleration_structure))
                .setMask(0xff)
                .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                .setTransform(transform<vk::TransformMatrixKHR, const glm::mat4 &>(matrix));
            instance_address_infos.push_back({
                .vertex_buffer_address = get_buffer_address(acceleration_structure->vertex_buffer->buffer),
                .index_buffer_address = get_buffer_address(acceleration_structure->index_buffer->buffer),
            });
        }
        vk::AccelerationStructureBuildRangeInfoKHR range {};
        range.setPrimitiveCount(model_infos.size())
            .setPrimitiveOffset(0)
            .setFirstVertex(0)
            .setTransformOffset(0);
        model_infos_buffer = std::make_unique<TwoStageBuffer>(instance_infos.size() * sizeof(vk::AccelerationStructureInstanceKHR), vk::BufferUsageFlags {}, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);
        model_infos_buffer->map();
        instance_addresses = std::make_shared<TwoStageBuffer>(instance_address_infos.size() * sizeof(InstanceAddress), vk::BufferUsageFlagBits::eStorageBuffer);
        model_infos_buffer->upload_data_from_vector(instance_infos);
        instance_addresses->upload_data_from_vector(instance_address_infos);

        vk::AccelerationStructureGeometryInstancesDataKHR instances {};
        instances.setData(get_buffer_address(model_infos_buffer->buffer->buffer));
        vk::AccelerationStructureGeometryKHR geometry {};
        geometry.setGeometry(instances)
            .setGeometryType(vk::GeometryTypeKHR::eInstances);

        vk::AccelerationStructureBuildGeometryInfoKHR build {};
        build.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
            .setGeometries(geometry);
        
        auto size_info = manager->device->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build, model_infos.size(), manager->dispatcher);
        instance_buffer = std::make_shared<Buffer>(size_info.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        scratch_buffer = std::make_unique<Buffer>(size_info.buildScratchSize, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        
        vk::AccelerationStructureCreateInfoKHR instance_create_info {};
        instance_create_info.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setSize(size_info.accelerationStructureSize)
            .setBuffer(instance_buffer->buffer);
        instance = manager->device->device.createAccelerationStructureKHR(instance_create_info, nullptr, manager->dispatcher);

        auto command_buffer = manager->command_pool->allocate_single_use();
        build.setDstAccelerationStructure(instance)
            .setScratchData(get_buffer_address(scratch_buffer->buffer));
        command_buffer.buildAccelerationStructuresKHR(build, &range, manager->dispatcher);
        manager->command_pool->free_single_use(command_buffer);
        return *this;
    }

    RayTracingInstance &RayTracingInstance::update(update_func func) {
        auto update = [&](uint32_t begin, uint32_t end) {
            auto current_instance_info = instance_infos.begin();
            for (uint32_t i = 0; i < begin; i ++) {
                current_instance_info ++;
            }
            interface_update_transform_func interface_update_transform = [&] (const glm::mat4 &transform_) {
                current_instance_info->setTransform(transform<vk::TransformMatrixKHR, const glm::mat4 &>(transform_));
            };
            while (begin < end) {
                func(begin, interface_update_transform);
                current_instance_info += 1;
                begin += 1;
            }
        };
        uint32_t size = instance_infos.size();
        uint32_t current = 0;
        std::vector<std::thread> thread_pool;
        while (current < size) {
            uint32_t end = std::min(current + 1000, size);
            thread_pool.push_back(std::move(std::thread ([&]() {
                update(current, end);
            })));
            current = end;
        }
        for (auto &thread : thread_pool) {
            thread.join();
        }
        
        model_infos_buffer->upload_data_from_vector(instance_infos);

        vk::AccelerationStructureGeometryInstancesDataKHR instances {};
        instances.setData(get_buffer_address(model_infos_buffer->buffer->buffer));
        vk::AccelerationStructureGeometryKHR geometry {};
        geometry.setGeometry(instances)
            .setGeometryType(vk::GeometryTypeKHR::eInstances);

        vk::AccelerationStructureBuildGeometryInfoKHR build {};
        build.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setMode(vk::BuildAccelerationStructureModeKHR::eUpdate)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
            .setGeometries(geometry);
        
        auto size_info = manager->device->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build, model_infos.size(), manager->dispatcher);
        if (size_info.buildScratchSize > scratch_buffer->size) {
            scratch_buffer.reset();
            scratch_buffer = std::make_unique<Buffer>(size_info.buildScratchSize, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        }
        
        auto command_buffer = manager->command_pool->allocate_single_use();
        build.setSrcAccelerationStructure(instance)
            .setDstAccelerationStructure(instance)
            .setScratchData(get_buffer_address(scratch_buffer->buffer));
        vk::AccelerationStructureBuildRangeInfoKHR range {};
        range.setPrimitiveCount(instance_infos.size())
            .setPrimitiveOffset(0)
            .setFirstVertex(0)
            .setTransformOffset(0);
        command_buffer.buildAccelerationStructuresKHR(build, &range, manager->dispatcher);
        manager->command_pool->free_single_use(command_buffer);
        return *this;
    }

    RayTracingInstance::~RayTracingInstance() {
        manager->device->device.destroyAccelerationStructureKHR(instance, nullptr, manager->dispatcher);
        instance_buffer.reset();
        scratch_buffer.reset();
        instance_addresses.reset();
        model_infos_buffer.reset();
    }
}
