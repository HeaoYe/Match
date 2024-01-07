#pragma once

#include <Match/vulkan/resource/buffer.hpp>

namespace Match {
    class Model;

    class ModelAccelerationStructure {
        default_no_copy_move_construction(ModelAccelerationStructure);
    public:
        ~ModelAccelerationStructure();
    INNER_VISIBLE:
        vk::AccelerationStructureKHR bottom_level_acceleration_structure ;
        std::unique_ptr<Buffer> acceleration_structure_buffer;
        std::unique_ptr<Buffer> vertex_buffer;
        std::unique_ptr<Buffer> index_buffer;
    };

    class AccelerationStructureBuilder {
        default_no_copy_move_construction(AccelerationStructureBuilder)
    private:
        struct BuildInfo {
            vk::AccelerationStructureGeometryTrianglesDataKHR triangles {};
            vk::AccelerationStructureGeometryKHR geometry {};
            vk::AccelerationStructureBuildGeometryInfoKHR build {};
            vk::AccelerationStructureBuildRangeInfoKHR range {};
            vk::AccelerationStructureBuildSizesInfoKHR size {};
            vk::AccelerationStructureKHR uncompacted_acceleration_structure {};
            std::unique_ptr<Buffer> uncompacted_acceleration_structure_buffer {};
        };
    public:
        void add_model(std::shared_ptr<Model> model);
        void build();
        ~AccelerationStructureBuilder();
    INNER_VISIBLE:
        std::unique_ptr<Buffer> staging;
        uint64_t current_staging_size = 0;
        std::unique_ptr<Buffer> scratch;
        uint64_t current_scratch_size = 0;
        std::vector<std::shared_ptr<Model>> models;
    };
}
