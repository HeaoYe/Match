#include "utils.hpp"


std::shared_ptr<Match::VertexBuffer> vertex_buffer;
std::shared_ptr<Match::IndexBuffer> index_buffer;

class BLAS {
public:
    BLAS(std::shared_ptr<Match::Model> model) {
        model->upload_data(vertex_buffer, index_buffer);
        std::vector<vk::AccelerationStructureGeometryKHR> geometries;
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR> ranges;
        std::vector<uint32_t> max_primitive_counts;
        for (auto &[name, mesh] : model->meshes) {
            max_primitive_counts.push_back(mesh->get_index_count() / 3);
            vk::AccelerationStructureGeometryTrianglesDataKHR triangles {};
            triangles.setVertexFormat(vk::Format::eR32G32B32Sfloat)
                .setVertexStride(sizeof(Match::Vertex))
                .setVertexData(get_address(*vertex_buffer->buffer))
                .setMaxVertex(model->vertex_count - 1)
                .setIndexType(vk::IndexType::eUint32)
                .setIndexData(get_address(*index_buffer->buffer));
            auto &geometry = geometries.emplace_back();
            geometry.setGeometry(triangles)
                .setGeometryType(vk::GeometryTypeKHR::eTriangles)
                .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
            auto &range = ranges.emplace_back();
            range.setFirstVertex(mesh->position.vertex_buffer_offset)
                .setPrimitiveOffset(mesh->position.index_buffer_offset / 3)
                .setPrimitiveCount(max_primitive_counts.back())
                .setTransformOffset(0);
        }
        vk::AccelerationStructureBuildGeometryInfoKHR build {};
        build.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction | vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
            .setGeometries(geometries);

        auto size_info = ctx->device->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build, max_primitive_counts, ctx->dispatcher);

        Match::Buffer scratch_buffer(size_info.buildScratchSize, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        auto scratch_addr = get_address(scratch_buffer);

        vk::QueryPoolCreateInfo query_pool_create_info {};
        query_pool_create_info.setQueryCount(1)
            .setQueryType(vk::QueryType::eAccelerationStructureCompactedSizeKHR);
        auto query_pool = ctx->device->device.createQueryPool(query_pool_create_info);
        vk::AccelerationStructureCreateInfoKHR as_create_info {};
        Match::Buffer temp_blas_buffer (size_info.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        as_create_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setSize(size_info.accelerationStructureSize)
            .setBuffer(temp_blas_buffer.buffer);
        auto temp_blas = ctx->device->device.createAccelerationStructureKHR(as_create_info, nullptr, ctx->dispatcher);

        auto cmd_buf = ctx->command_pool->allocate_single_use();
        build.setDstAccelerationStructure(temp_blas)
            .setScratchData(scratch_addr);
        cmd_buf.buildAccelerationStructuresKHR(build, ranges.data(), ctx->dispatcher);
        ctx->command_pool->free_single_use(cmd_buf);

        cmd_buf = ctx->command_pool->allocate_single_use();
        cmd_buf.resetQueryPool(query_pool, 0, 1);
        cmd_buf.writeAccelerationStructuresPropertiesKHR(temp_blas, vk::QueryType::eAccelerationStructureCompactedSizeKHR, query_pool, 0, ctx->dispatcher);
        ctx->command_pool->free_single_use(cmd_buf);

        auto results = ctx->device->device.getQueryPoolResult<uint32_t>(query_pool, 0, 1, sizeof(vk::DeviceSize), vk::QueryResultFlagBits::eWait);
        auto compacted_size = results.value;
        MCH_INFO("{} -> {}", size_info.accelerationStructureSize, compacted_size);

        Match::Buffer blas_buffer (size_info.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        as_create_info.setSize(compacted_size)
            .setBuffer(blas_buffer.buffer);
        blas = ctx->device->device.createAccelerationStructureKHR(as_create_info, nullptr, ctx->dispatcher);
        
        cmd_buf = ctx->command_pool->allocate_single_use();
        vk::CopyAccelerationStructureInfoKHR copy {};
        copy.setSrc(temp_blas)
            .setDst(blas)
            .setMode(vk::CopyAccelerationStructureModeKHR::eCompact);
        cmd_buf.copyAccelerationStructureKHR(copy, ctx->dispatcher);
        ctx->command_pool->free_single_use(cmd_buf);

        ctx->device->device.destroyAccelerationStructureKHR(temp_blas, nullptr, ctx->dispatcher);
        ctx->device->device.destroyQueryPool(query_pool);
    }

    ~BLAS() {
        ctx->device->device.destroyAccelerationStructureKHR(blas, nullptr, ctx->dispatcher);
    }
private:
    vk::AccelerationStructureKHR blas;
};


class RayTracingApplication {
public:
    RayTracingApplication() {
        Match::setting.debug_mode = true;
        Match::setting.max_in_flight_frame = 1;
        Match::setting.enable_ray_tracing = true;
        ctx = &Match::Initialize();
        factory = ctx->create_resource_factory("../Scene/resource");
        auto additional_usages = vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;
        vertex_buffer = factory->create_vertex_buffer(sizeof(Match::VertexBuffer), 4096000, additional_usages);
        index_buffer = factory->create_index_buffer(Match::IndexType::eUint32, 4096000, additional_usages);
        create_as();
    }

    ~RayTracingApplication() {
        vertex_buffer.reset();
        index_buffer.reset();
        Match::Destroy();
    }

    void create_as() {
        auto model = factory->load_model("mori_knob.obj");
        // auto model = factory->load_model("dragon.obj");
        BLAS a(model);
    }

    void gameloop() {
        while (Match::window->is_alive()) {
            Match::window->poll_events();
        }
    }
private:
    std::shared_ptr<Match::ResourceFactory> factory;
};


int main() {
    RayTracingApplication app {};
    app.gameloop();
}
