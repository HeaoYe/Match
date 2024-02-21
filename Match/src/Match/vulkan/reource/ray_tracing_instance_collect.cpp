#include <Match/vulkan/resource/ray_tracing_instance_collect.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"

namespace Match {
    RayTracingInstanceCollect::RayTracingInstanceCollect(bool allow_update) : allow_update(allow_update), instance_count(0) {
        registrar = std::make_unique<CustomDataRegistrar<InstanceCreateInfo>>();
        registrar->register_custom_data<InstanceAddressData>();
    }

    RayTracingInstanceCollect &RayTracingInstanceCollect::build() {
        instance_count = registrar->build_groups();

        acceleration_struction_instance_infos_buffer = std::make_unique<Buffer>(instance_count * sizeof(vk::AccelerationStructureInstanceKHR), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        auto *acceleration_struction_instance_infos_ptr = static_cast<vk::AccelerationStructureInstanceKHR *>(acceleration_struction_instance_infos_buffer->map());

        for (auto &[group_id, group_info] : registrar->groups) {
            uint32_t instance_index = group_info.offset;
            auto *current_acceleration_struction_instance_infos_ptr = acceleration_struction_instance_infos_ptr + instance_index;
            for (auto &[model, transform_matrix, hit_group] : group_info.custom_create_infos) {
                current_acceleration_struction_instance_infos_ptr
                    ->setInstanceCustomIndex(instance_index)
                    .setAccelerationStructureReference(get_acceleration_structure_address(model->acceleration_structure.value()->bottom_level_acceleration_structure))
                    .setInstanceShaderBindingTableRecordOffset(hit_group)
                    .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                    .setMask(0xff)
                    .setTransform(transform<vk::TransformMatrixKHR, const glm::mat4 &>(transform_matrix));
                current_acceleration_struction_instance_infos_ptr ++;
                instance_index ++;
            }
            group_info.custom_create_infos.clear();
        }

        vk::AccelerationStructureBuildRangeInfoKHR range {};
        range.setPrimitiveCount(instance_count)
            .setPrimitiveOffset(0)
            .setFirstVertex(0)
            .setTransformOffset(0);

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

    RayTracingInstanceCollect &RayTracingInstanceCollect::update(uint32_t group_id, UpdateCallback update_callback) {
        if (!allow_update) {
            MCH_WARN("This RayTracingInstanceCollect doesn't allow update")
            return *this;
        }

        registrar->multithread_update(group_id, [=](uint32_t in_group_index, uint32_t batch_begin, uint32_t batch_end) {
            auto *acceleration_struction_instance_infos_ptr = static_cast<vk::AccelerationStructureInstanceKHR *>(acceleration_struction_instance_infos_buffer->data_ptr);
            acceleration_struction_instance_infos_ptr += batch_begin;
            InterfaceSetTransform interface_set_transform = [&](const glm::mat4 &transform_matrix) {
                acceleration_struction_instance_infos_ptr->setTransform(transform<vk::TransformMatrixKHR, const glm::mat4 &>(transform_matrix));
            };
            InterfaceUpdataAccelerationStructure interface_update_acceleration_structure = [&](std::shared_ptr<RayTracingModel> model) {
                acceleration_struction_instance_infos_ptr->setAccelerationStructureReference(get_acceleration_structure_address(model->acceleration_structure.value()->bottom_level_acceleration_structure));
            };
            while (batch_begin < batch_end) {
                update_callback(in_group_index, interface_set_transform, interface_update_acceleration_structure);
                acceleration_struction_instance_infos_ptr ++;
                in_group_index ++;
                batch_begin ++;
            }
        });

        auto &group_info = registrar->groups.at(group_id);

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

        auto size_info = manager->device->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build, instance_count, manager->dispatcher);
        if (scratch_buffer->size < size_info.updateScratchSize) {
            scratch_buffer.reset();
            scratch_buffer = std::make_unique<Buffer>(size_info.updateScratchSize, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        }

        auto command_buffer = manager->command_pool->allocate_single_use();
        build.setSrcAccelerationStructure(instance_collect)
            .setDstAccelerationStructure(instance_collect)
            .setScratchData(get_buffer_address(scratch_buffer->buffer));
        vk::AccelerationStructureBuildRangeInfoKHR range {};
        range.setPrimitiveCount(instance_count)
            .setPrimitiveOffset(0)
            .setFirstVertex(0)
            .setTransformOffset(0);
        command_buffer.buildAccelerationStructuresKHR(build, &range, manager->dispatcher);
        manager->command_pool->free_single_use(command_buffer);

        return *this;
    }

    RayTracingInstanceCollect::~RayTracingInstanceCollect() {
        manager->device->device.destroyAccelerationStructureKHR(instance_collect, nullptr, manager->dispatcher);
        instance_collect_buffer.reset();
        scratch_buffer.reset();
        acceleration_struction_instance_infos_buffer.reset();
        registrar.reset();
    }

    RayTracingInstanceCollect::InstanceAddressData RayTracingInstanceCollect::create_instance_address_data(RayTracingModel &model) {
        switch (model.get_ray_tracing_model_type()) {
        case RayTracingModel::RayTracingModelType::eModel:
            return {
                .vertex_buffer_address = get_buffer_address(dynamic_cast<Model &>(model).vertex_buffer->buffer),
                .index_buffer_address = get_buffer_address(dynamic_cast<Model &>(model).index_buffer->buffer),
            };
        case RayTracingModel::RayTracingModelType::eSphereCollect:
            return {
                .vertex_buffer_address = 0,
                .index_buffer_address = 0,
            };
        case RayTracingModel::RayTracingModelType::eGLTFPrimitive:
            return {
                .vertex_buffer_address = get_buffer_address(dynamic_cast<GLTFPrimitive &>(model).scene.vertex_buffer->buffer),
                .index_buffer_address = get_buffer_address(dynamic_cast<GLTFPrimitive &>(model).scene.index_buffer->buffer),
            };
        }
        return {};
    }

    bool RayTracingInstanceCollect::check_model_suitable(RayTracingModel &model) {
        if (model.acceleration_structure.has_value()) {
            return true;
        }
        MCH_WARN("Can't add the model which doesn't have acceleration structure")
        return false;
    }

}
