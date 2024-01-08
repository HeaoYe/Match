#include <Match/vulkan/resource/model_acceleration_structure.hpp>
#include <Match/vulkan/resource/model.hpp>
#include <Match/vulkan/utils.hpp>
#include "../inner.hpp"

namespace Match {
    ModelAccelerationStructure::~ModelAccelerationStructure() {
        manager->device->device.destroyAccelerationStructureKHR(bottom_level_acceleration_structure, nullptr, manager->dispatcher);
        acceleration_structure_buffer.reset();
        vertex_buffer.reset();
        index_buffer.reset();
    }

    void AccelerationStructureBuilder::add_model(std::shared_ptr<Model> model) {
        if (model->acceleration_structure.has_value()) {
            MCH_WARN("Model has built acceleration structure")
            return;
        }
        models.push_back(std::move(model));
    }

    void AccelerationStructureBuilder::build() {
        std::vector<BuildInfo> build_infos;
        build_infos.reserve(models.size());
        uint64_t max_staging_size = current_staging_size, max_scratch_size = current_scratch_size;
        for (auto &model : models) {
            auto model_acceleration_structure = std::make_unique<ModelAccelerationStructure>();
            auto vertices_size = model->vertex_count * sizeof(Vertex);
            auto indices_size = model->index_count * sizeof(uint32_t);
            auto primitive_count = model->index_count / 3;
            model_acceleration_structure->vertex_buffer = std::make_unique<Buffer>(vertices_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
            model_acceleration_structure->index_buffer = std::make_unique<Buffer>(indices_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
            max_staging_size = std::max(max_staging_size, vertices_size);
            max_staging_size = std::max(max_staging_size, indices_size);
            auto &info = build_infos.emplace_back();

            info.triangles.setVertexFormat(vk::Format::eR32G32B32Sfloat)
                .setVertexStride(sizeof(Match::Vertex))
                .setVertexData(get_buffer_address(model_acceleration_structure->vertex_buffer->buffer))
                .setMaxVertex(model->vertex_count - 1)
                .setIndexType(vk::IndexType::eUint32)
                .setIndexData(get_buffer_address(model_acceleration_structure->index_buffer->buffer));
            info.geometry.setGeometry(info.triangles)
                .setGeometryType(vk::GeometryTypeKHR::eTriangles)
                .setFlags(vk::GeometryFlagBitsKHR::eOpaque);

            info.range.setFirstVertex(0)
                .setPrimitiveOffset(0)
                .setPrimitiveCount(primitive_count)
                .setTransformOffset(0);
            info.build.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
                .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
                .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction | vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
                .setGeometries(info.geometry);
            info.size = manager->device->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, info.build, primitive_count, manager->dispatcher);
            max_scratch_size = std::max(max_scratch_size, info.size.buildScratchSize);
            model->acceleration_structure = std::move(model_acceleration_structure);
        }
        
        if (max_staging_size > current_staging_size) {
            staging.reset();
            staging = std::make_unique<Buffer>(max_staging_size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
            current_staging_size = max_staging_size;
        }
        if (max_scratch_size > current_scratch_size) {
            scratch.reset();
            scratch = std::make_unique<Buffer>(max_scratch_size, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
            current_scratch_size = max_scratch_size;
        }
        auto scratch_address = get_buffer_address(scratch->buffer);

        for (auto &model : models) {
            auto *ptr = static_cast<uint8_t *>(staging->map());
            uint64_t vertices_size = model->vertex_count * sizeof(Vertex);

            memcpy(ptr, model->vertices.data(), vertices_size);
            auto copy_command_buffer = manager->command_pool->allocate_single_use();
            vk::BufferCopy copy {};
            copy.setSrcOffset(0)
                .setDstOffset(0)
                .setSize(vertices_size);
            copy_command_buffer.copyBuffer(staging->buffer, model->acceleration_structure.value()->vertex_buffer->buffer, copy);
            manager->command_pool->free_single_use(copy_command_buffer);

            uint64_t total_indices_size = 0;
            for (const auto &[name, mesh] : model->meshes) {
                uint64_t indices_size = mesh->indices.size() * sizeof(uint32_t);
                memcpy(ptr, mesh->indices.data(), indices_size);
                ptr += indices_size;
                total_indices_size += indices_size;
            }
            copy_command_buffer = manager->command_pool->allocate_single_use();
            copy.setSize(total_indices_size);
            copy_command_buffer.copyBuffer(staging->buffer, model->acceleration_structure.value()->index_buffer->buffer, copy);
            manager->command_pool->free_single_use(copy_command_buffer);
        }
        staging->unmap();
        
        vk::QueryPoolCreateInfo query_pool_create_info {};
        query_pool_create_info.setQueryCount(build_infos.size())
            .setQueryType(vk::QueryType::eAccelerationStructureCompactedSizeKHR);
        auto query_pool = manager->device->device.createQueryPool(query_pool_create_info);

        std::vector<uint32_t> build_info_indices;
        uint64_t batch_size = 0;
        for (uint32_t info_index = 0; info_index < build_infos.size(); info_index ++) {
            batch_size += build_infos[info_index].size.accelerationStructureSize;
            build_info_indices.push_back(info_index);
            if (batch_size > 256 * 1024 * 1024 || info_index == build_infos.size() - 1) {
                uint32_t query_index = 0;
                auto command_buffer = manager->command_pool->allocate_single_use();
                command_buffer.resetQueryPool(query_pool, 0, models.size());
                for (auto index : build_info_indices) {
                    auto &info = build_infos[index];
                    info.uncompacted_acceleration_structure_buffer = std::make_unique<Buffer>(info.size.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

                    vk::AccelerationStructureCreateInfoKHR acceleration_structure_create_info {};
                    acceleration_structure_create_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
                        .setBuffer(info.uncompacted_acceleration_structure_buffer->buffer)
                        .setSize(info.size.accelerationStructureSize);
                    info.uncompacted_acceleration_structure = manager->device->device.createAccelerationStructureKHR(acceleration_structure_create_info, nullptr, manager->dispatcher);

                    info.build.setDstAccelerationStructure(info.uncompacted_acceleration_structure)
                        .setScratchData(scratch_address);
                    command_buffer.buildAccelerationStructuresKHR(info.build, &info.range, manager->dispatcher);

                    vk::MemoryBarrier barrier {};
                    barrier.setSrcAccessMask(vk::AccessFlagBits::eAccelerationStructureWriteKHR)
                        .setDstAccessMask(vk::AccessFlagBits::eAccelerationStructureReadKHR);
                    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, vk::DependencyFlags {}, barrier, {}, {});

                    command_buffer.writeAccelerationStructuresPropertiesKHR(info.uncompacted_acceleration_structure, vk::QueryType::eAccelerationStructureCompactedSizeKHR, query_pool, query_index, manager->dispatcher);
                    query_index += 1;
                }
                manager->command_pool->free_single_use(command_buffer);

                std::vector<vk::DeviceSize> compacted_sizes(build_info_indices.size());
                vk_check(manager->device->device.getQueryPoolResults(query_pool, 0, compacted_sizes.size(), sizeof(vk::DeviceSize) * compacted_sizes.size(), compacted_sizes.data(), sizeof(vk::DeviceSize), vk::QueryResultFlagBits::eWait));

                command_buffer = manager->command_pool->allocate_single_use();
                uint32_t compacted_sizes_index = 0;
                for (auto index : build_info_indices) {
                    auto &info = build_infos[index];
                    auto &acceleration_structure = models[index]->acceleration_structure.value();
                    acceleration_structure->acceleration_structure_buffer = std::make_unique<Buffer>(compacted_sizes[compacted_sizes_index], vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

                    vk::AccelerationStructureCreateInfoKHR acceleration_structure_create_info {};
                    acceleration_structure_create_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
                        .setBuffer(acceleration_structure->acceleration_structure_buffer->buffer)
                        .setSize(compacted_sizes[compacted_sizes_index]);
                    acceleration_structure->bottom_level_acceleration_structure = manager->device->device.createAccelerationStructureKHR(acceleration_structure_create_info, nullptr, manager->dispatcher);

                    vk::CopyAccelerationStructureInfoKHR compact {};
                    compact.setSrc(info.uncompacted_acceleration_structure)
                        .setDst(acceleration_structure->bottom_level_acceleration_structure)
                        .setMode(vk::CopyAccelerationStructureModeKHR::eCompact);
                    command_buffer.copyAccelerationStructureKHR(compact, manager->dispatcher);

                    compacted_sizes_index += 1;
                }
                manager->command_pool->free_single_use(command_buffer);
                build_info_indices.clear();
                batch_size = 0;
            }
        }
        for (auto &info : build_infos) {
            info.uncompacted_acceleration_structure_buffer.reset();
            manager->device->device.destroyAccelerationStructureKHR(info.uncompacted_acceleration_structure, nullptr, manager->dispatcher);
        }
        build_infos.clear();
        manager->device->device.destroyQueryPool(query_pool);
        models.clear();
    }

    AccelerationStructureBuilder::~AccelerationStructureBuilder() {
        staging.reset();
        scratch.reset();
        models.clear();
    }
}