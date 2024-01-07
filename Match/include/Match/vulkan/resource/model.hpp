#pragma once

#include <Match/vulkan/resource/buffer.hpp>
#include <Match/vulkan/resource/vertex_attribute_set.hpp>
#include <Match/vulkan/resource/model_acceleration_structure.hpp>
#include <optional>

namespace Match {
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
 
        bool operator==(const Vertex& rhs) const {
            return pos == rhs.pos && normal == rhs.normal && color == rhs.color;
        }
        static InputBindingInfo generate_input_binding(uint32_t binding);
    };

    struct BufferPosition {
        uint32_t vertex_buffer_offset;
        uint32_t index_buffer_offset;
    };

    class Mesh {
        no_copy_move_construction(Mesh)
    public:
        Mesh();
        ~Mesh();
        uint32_t get_index_count() const { return indices.size(); }
    INNER_VISIBLE:
        BufferPosition position;
        std::vector<uint32_t> indices;
    };

    class Model {
        no_copy_move_construction(Model)
    public:
        Model();
        Model(const std::string &filename);
        BufferPosition upload_data(std::shared_ptr<VertexBuffer> vertex_buffer, std::shared_ptr<IndexBuffer> index_buffer, BufferPosition position = { 0, 0 });
        std::shared_ptr<const Mesh> get_mesh_by_name(const std::string &name) const;
        std::vector<std::string> enumerate_meshes_name() const;
        uint32_t get_vertex_count() const { return vertex_count; }
        uint32_t get_index_count() const { return index_count; }
        ~Model();
    INNER_VISIBLE:
        BufferPosition position;
        std::vector<Vertex> vertices;
        uint32_t vertex_count;
        uint32_t index_count;
        std::map<std::string, std::shared_ptr<Mesh>> meshes;
        std::optional<std::unique_ptr<ModelAccelerationStructure>> acceleration_structure;
    };
}
