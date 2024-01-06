#include <Match/vulkan/resource/model.hpp>
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std {
    template<> struct hash<Match::Vertex> {
        size_t operator()(Match::Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                   (hash<glm::vec3>()(vertex.color) << 1);
        }
    };
}

namespace Match {
    InputBindingInfo Vertex::generate_input_binding(uint32_t binding) {
        return {
            .binding = binding,
            .rate = InputRate::ePerVertex,
            .attributes = { VertexType::eFloat3, VertexType::eFloat3, VertexType::eFloat3 }
        };
    }

    Mesh::Mesh() {
    }

    Mesh::~Mesh() {
        indices.clear();
    }

    Model::Model() {
        vertex_count = 0;
        index_count = 0;
    }

    Model::Model(const std::string &filename) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) {
            MCH_ERROR("Faild load model: {}", filename)
            return;
        }

        uint32_t size = 0;
        for (const auto& shape : shapes) {
            size += shape.mesh.indices.size();
        }

        std::unordered_map<Vertex, uint32_t> unique_vertices;
        unique_vertices.reserve(size);
        vertices.reserve(size);

        for (const auto& shape : shapes) {
            std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
            mesh->indices.reserve(shape.mesh.indices.size());
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex {};
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
                vertex.color = { 1, 1, 1 };

                if (unique_vertices.count(vertex) == 0) {
                    unique_vertices.insert(std::make_pair(vertex, vertices.size()));
                    vertices.push_back(vertex);
                }
                mesh->indices.push_back(unique_vertices.at(vertex));
            }
            index_count += mesh->get_index_count();
            meshes.insert(std::make_pair(shape.name, std::move(mesh)));
        }
        vertex_count = vertices.size();
    }

    Model::~Model() {
        vertices.clear();
        meshes.clear();
    }

    BufferPosition Model::upload_data(std::shared_ptr<VertexBuffer> vertex_buffer, std::shared_ptr<IndexBuffer> index_buffer, BufferPosition position) {
        this->position = position;
        auto temp_position = position;
        temp_position.vertex_buffer_offset = vertex_buffer->upload_data_from_vector(vertices, temp_position.vertex_buffer_offset);
        for (auto &[name, mesh] : meshes) {
            mesh->position.vertex_buffer_offset = this->position.vertex_buffer_offset;
            mesh->position.index_buffer_offset = temp_position.index_buffer_offset;
            temp_position.index_buffer_offset = index_buffer->upload_data_from_vector(mesh->indices, temp_position.index_buffer_offset);
        }
        return temp_position;
    }

    std::shared_ptr<const Mesh> Model::get_mesh_by_name(const std::string &name) const {
        return meshes.at(name);
    }

    std::vector<std::string> Model::enumerate_meshes_name() const {
        std::vector<std::string> result;
        result.reserve(meshes.size());
        for (auto &[name, mesh] : meshes) {
            result.push_back(name);
        }
        return result;
    }
}
