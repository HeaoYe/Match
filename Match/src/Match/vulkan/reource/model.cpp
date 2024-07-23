#include <Match/vulkan/resource/model.hpp>
#include <rapidobj/rapidobj.hpp>
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

    RayTracingModel::~RayTracingModel() {
        if (acceleration_structure.has_value()) {
            acceleration_structure.value().reset();
            acceleration_structure->reset();
        }
    }

    Model::Model(const std::string &filename) : vertex_count(0), index_count(0) {
        rapidobj::Result parsed_obj = rapidobj::ParseFile(filename, rapidobj::MaterialLibrary::Ignore());

        uint32_t size = 0;
        for (const auto& shape : parsed_obj.shapes) {
            size += shape.mesh.indices.size();
        }

        std::unordered_map<Vertex, uint32_t> unique_vertices;
        unique_vertices.reserve(size);
        vertices.reserve(size);

        for (const auto& shape : parsed_obj.shapes) {
            std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
            mesh->indices.reserve(shape.mesh.indices.size());
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex {};
                vertex.pos = {
                    parsed_obj.attributes.positions[3 * index.position_index + 0],
                    parsed_obj.attributes.positions[3 * index.position_index + 1],
                    parsed_obj.attributes.positions[3 * index.position_index + 2]
                };
                vertex.normal = {
                    parsed_obj.attributes.normals[3 * index.normal_index + 0],
                    parsed_obj.attributes.normals[3 * index.normal_index + 1],
                    parsed_obj.attributes.normals[3 * index.normal_index + 2]
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
        vertex_buffer.reset();
        index_buffer.reset();
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

    SphereCollect::SphereCollect() {
        registrar = std::make_unique<CustomDataRegistrar<uint8_t>>();
        registrar->register_custom_data<SphereData>();
        builder = std::make_unique<AccelerationStructureBuilder>();
    }

    SphereCollect &SphereCollect::build() {
        uint32_t count = registrar->build_groups();
        if (count == 0) {
            MCH_ERROR("Can't build an empty sphere collect")
            return *this;
        }

        aabbs.reserve(count);
        auto *ptr = static_cast<SphereData *>(get_spheres_buffer()->map());
        for (uint32_t i = 0; i < count; i ++) {
            aabbs.push_back({
                .minium = ptr->center - ptr->radius,
                .maxium = ptr->center + ptr->radius,
            });
            ptr ++;
        }
        get_spheres_buffer()->unmap();

        aabbs_buffer = std::make_shared<Buffer>(aabbs.size() * sizeof(SphereAaBbData), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        memcpy(aabbs_buffer->map(), aabbs.data(), aabbs_buffer->size);
        aabbs_buffer->unmap();

        return *this;
    }

    SphereCollect &SphereCollect::update(uint32_t group_id, UpdateCustomDataCallback<SphereData> update_callback) {
        registrar->multithread_update(group_id, [=](uint32_t in_group_index, uint32_t batch_begin, uint32_t batch_end) {
            auto *sphere_data_ptr = static_cast<SphereData *>(registrar->get_custom_data_buffer<SphereData>()->map());
            auto *aabb_data_ptr = static_cast<SphereAaBbData *>(aabbs_buffer->map());
            aabb_data_ptr += batch_begin;
            sphere_data_ptr += batch_begin;
            while (batch_begin < batch_end) {
                update_callback(in_group_index, *sphere_data_ptr);
                aabb_data_ptr->minium = sphere_data_ptr->center - sphere_data_ptr->radius;
                aabb_data_ptr->maxium = sphere_data_ptr->center + sphere_data_ptr->radius;
                sphere_data_ptr ++;
                aabb_data_ptr ++;
                in_group_index ++;
                batch_begin ++;
            }
        });
        std::shared_ptr<SphereCollect> sphere_collect(this, [](auto) {});
        builder->add_model(sphere_collect);
        builder->update();
        return *this;
    }

    SphereCollect::~SphereCollect() {
        registrar.reset();
    }
}
