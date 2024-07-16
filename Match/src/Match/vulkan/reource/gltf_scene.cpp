#include <Match/vulkan/resource/gltf_scene.hpp>
#include <Match/vulkan/descriptor_resource/spec_texture.hpp>
#include <Match/vulkan/descriptor_resource/descriptor_set.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Match {
    GLTFScene::GLTFScene(const std::string &filename, const std::vector<std::string> &load_attributes) {
        tinygltf::TinyGLTF loader;
        tinygltf::Model gltf_model;
        std::string err, warn;

        size_t pos = filename.find_last_of('/');
        path = filename.substr(0, pos);

        auto filetype = filename.substr(filename.find_last_of('.') + 1);
        bool ret = false;
        if (filetype == "gltf") {
            ret = loader.LoadASCIIFromFile(&gltf_model, &err, &warn, filename);
        } else if (filetype == "glb") {
            ret = loader.LoadBinaryFromFile(&gltf_model, &err, &warn, filename);
        } else {
            MCH_ERROR("Unknown GLTF Filetype {}", filename)
        }

        if (!warn.empty()) {
            MCH_WARN("Warn: {} {}", warn, filename);
        }
        if (!err.empty()) {
            MCH_ERROR("Err: {} {}", err, filename);
        }
        if (!ret) {
            MCH_FATAL("Failed to parse glTF {}", filename);
            return;
        }

        for (auto &extension : gltf_model.extensionsRequired) {
            MCH_INFO("GLTFScene Required {} extension", extension)
        }

        load_images(gltf_model);
        load_materials(gltf_model);
        material_buffer = std::make_shared<Buffer>(materials.size() * sizeof(GLTFMaterial), vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        memcpy(material_buffer->map(), materials.data(), material_buffer->size);
        material_buffer->unmap();

        for (auto &gltf_mesh : gltf_model.meshes) {
            meshes.push_back(std::make_shared<GLTFMesh>(*this, gltf_model, gltf_mesh, load_attributes));
        }

        for (auto &[attribute_name, data] : attribute_datas) {
            attribute_buffer[attribute_name] = std::make_shared<Buffer>(data.size(), vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
            memcpy(attribute_buffer[attribute_name]->map(), data.data(), data.size());
            attribute_buffer[attribute_name]->unmap();
        }

        auto gltf_default_scene = gltf_model.scenes[std::max(0, gltf_model.defaultScene)];

        for (uint32_t gltf_node_index : gltf_default_scene.nodes) {
            auto node = std::make_shared<GLTFNode>(*this, gltf_model, gltf_node_index, nullptr);
            all_node_references.push_back(node.get());
            nodes.push_back(std::move(node));
        }
    }

    void GLTFScene::enumerate_primitives(std::function<void(GLTFNode *, std::shared_ptr<GLTFPrimitive>)> func) {
        for (auto *node : all_node_references) {
            if (node->mesh.get() == nullptr) {
                continue;
            }
            for (auto &primitive : node->mesh->primitives) {
                func(node, primitive);
            }
        }
    }

    void GLTFScene::load_images(const tinygltf::Model &gltf_model) {
        for (auto &gltf_image : gltf_model.images) {
            if (gltf_image.uri.empty()) {
                MCH_WARN("Unsupported Image Format")
                continue;
            }

            std::string::size_type pos;
            if ((pos = gltf_image.uri.find_last_of('.')) != std::string::npos) {
                if (gltf_image.uri.substr(pos + 1) == "ktx") {
#if defined (MATCH_WITH_KTX)
                    textures.push_back(std::make_shared<Match::KtxTexture>(path + "/" + gltf_image.uri));
#else
                    MCH_ERROR("Please set MATCH_SUPPORT_KTX to ON to support ktx in gltf scene.")
#endif
                    continue;
                }
            }

            std::vector<uint8_t> rgba;
            const uint8_t *rgba_readonly = nullptr;
            if (gltf_image.component == 3) {
                rgba.resize(gltf_image.width * gltf_image.height * 4);
                auto *rgb = gltf_image.image.data();
                auto *ptr = rgba.data();
                for (uint32_t i = 0; i < gltf_image.width * gltf_image.height; i ++) {
                    ptr[0] = rgb[0];
                    ptr[1] = rgb[1];
                    ptr[2] = rgb[2];
                    ptr[3] = 0;
                    ptr += 4;
                    rgb += 3;
                }
                rgba_readonly = rgba.data();
                MCH_DEBUG("Convert RGB -> RGBA {} x {}", gltf_image.width, gltf_image.height)
            } else {
                assert(gltf_image.component == 4);
                rgba_readonly = gltf_image.image.data();
                MCH_DEBUG("Load RGBA {} x {}", gltf_image.width, gltf_image.height)
            }
            textures.push_back(std::make_shared<Match::DataTexture>(rgba_readonly, gltf_image.width, gltf_image.height, 0));
        }
        sampler = std::make_shared<Sampler>(SamplerOptions {});
    }

    void GLTFScene::load_materials(const tinygltf::Model &gltf_model) {
        for (auto &gltf_material : gltf_model.materials) {
            auto &material = materials.emplace_back();
            material.base_color_factor = glm::make_vec4(gltf_material.pbrMetallicRoughness.baseColorFactor.data());
            material.base_color_texture = gltf_material.pbrMetallicRoughness.baseColorTexture.index;
            material.emissive_factor = glm::make_vec3(gltf_material.emissiveFactor.data());
            material.emissive_texture = gltf_material.emissiveTexture.index;
            material.normal_texture = gltf_material.normalTexture.index;
            material.metallic_factor = gltf_material.pbrMetallicRoughness.metallicFactor;
            material.roughness_factor = gltf_material.pbrMetallicRoughness.roughnessFactor;
            material.metallic_roughness_texture = gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index;
        }
    }

    void GLTFScene::bind_textures(std::shared_ptr<DescriptorSet> descriptor_set, uint32_t binding) {
        std::vector<std::pair<std::shared_ptr<Texture>, std::shared_ptr<Sampler>>> textures_samplers;
        for (auto &texture : textures) {
            textures_samplers.push_back(std::make_pair(texture, sampler));
        }
        descriptor_set->bind_textures(binding, textures_samplers);
    }

    GLTFScene::~GLTFScene() {
        nodes.clear();
        meshes.clear();
        positions.clear();
        indices.clear();
        material_buffer.reset();
        materials.clear();
        sampler.reset();
        textures.clear();
        vertex_buffer.reset();
        index_buffer.reset();
    }

    GLTFNode::GLTFNode(GLTFScene &scene, const tinygltf::Model &gltf_model, uint32_t gltf_node_index, GLTFNode *parent) : parent(parent) {
        auto &gltf_node = gltf_model.nodes[gltf_node_index];

        name = gltf_node.name;

        if (gltf_node.mesh > -1) {
            mesh = scene.meshes[gltf_node.mesh];
        }
        if (gltf_node.rotation.size() == 4) {
            auto t_rotation = glm::make_quat(gltf_node.rotation.data());
            rotation.x = t_rotation.w;
            rotation.y = t_rotation.x;
            rotation.z = t_rotation.y;
            rotation.w = t_rotation.z;
        }
        if (gltf_node.scale.size() == 3) {
            scale = glm::make_vec3(gltf_node.scale.data());
        }
        if (gltf_node.translation.size() == 3) {
            translation = glm::make_vec3(gltf_node.translation.data());
        }
        if (gltf_node.matrix.size() == 16) {
            matrix = glm::make_mat4(gltf_node.matrix.data());
        }

        for (uint32_t gltf_child_index : gltf_node.children) {
            auto node = std::make_shared<GLTFNode>(scene, gltf_model, gltf_child_index, this);
            scene.all_node_references.push_back(node.get());
            children.push_back(std::move(node));
        }
    }

    glm::mat4 GLTFNode::get_local_matrix() {
        return glm::translate(glm::mat4(1), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1), scale) * matrix;
    }

    glm::mat4 GLTFNode::get_world_matrix() {
        if (parent != nullptr) {
            return parent->get_world_matrix() * get_local_matrix();
        } else {
            return get_local_matrix();
        }
    }

    GLTFNode::~GLTFNode() {
        children.clear();
    }

    GLTFMesh::GLTFMesh(GLTFScene &scene, const tinygltf::Model &gltf_model, const tinygltf::Mesh &gltf_mesh, const std::vector<std::string> &load_attributes) {
        name = gltf_mesh.name;
        MCH_DEBUG("Load Mesh {}", name)
        for (auto &gltf_primitive : gltf_mesh.primitives) {
            if (gltf_primitive.mode != TINYGLTF_MODE_TRIANGLES) {
                MCH_WARN("Unsupported Primitive Mode {}", gltf_primitive.mode)
                continue;
            }
            if (gltf_primitive.indices <= -1) {
                MCH_WARN("Unsupported Primitive Indices {}", gltf_primitive.indices)
                continue;
            }
            primitives.push_back(std::make_unique<GLTFPrimitive>(scene, gltf_model, gltf_primitive, load_attributes));
        }
    }

    GLTFMesh::~GLTFMesh() {
        primitives.clear();
    }

    GLTFPrimitive::GLTFPrimitive(GLTFScene &scene, const tinygltf::Model &gltf_model, const tinygltf::Primitive &gltf_primitive, const std::vector<std::string> &load_attributes) : scene(scene) {
        primitive_instance_data.first_index = scene.indices.size();
        primitive_instance_data.first_vertex = scene.positions.size();
        primitive_instance_data.material_index = gltf_primitive.material;

        auto no_operator = [](const tinygltf::Accessor &) {};
        auto get_data_pointer = [&](const std::string &name, auto other_operator) {
            if (gltf_primitive.attributes.find(name) == gltf_primitive.attributes.end()) {
                MCH_ERROR("Attribute {} Not Found", name)
                return static_cast<const void *>(nullptr);
            }
            const auto &accessor = gltf_model.accessors[gltf_primitive.attributes.find(name)->second];
            const auto &buffer_view = gltf_model.bufferViews[accessor.bufferView];
            other_operator(accessor);
            return reinterpret_cast<const void *>(gltf_model.buffers[buffer_view.buffer].data.data() + accessor.byteOffset + buffer_view.byteOffset);
        };

        auto *position_ptr = static_cast<const float *>(get_data_pointer("POSITION", [&](auto &accessor) {
            vertex_count = accessor.count;
            pos_min = glm::make_vec3(accessor.minValues.data());
            pos_max = glm::make_vec3(accessor.maxValues.data());
        }));
        for (uint32_t i = 0; i < vertex_count; i++) {
            scene.positions.push_back(glm::make_vec3(&position_ptr[i * 3]));
        }
        for (auto &attribute_name : load_attributes) {
            uint32_t count = 0;
            uint32_t stride = 0;
            auto *ptr = static_cast<const uint8_t *>(get_data_pointer(attribute_name, [&](auto &accessor) {
                count = accessor.count;
                switch (accessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    stride = sizeof(double);
                    break;
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                case TINYGLTF_COMPONENT_TYPE_INT:
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    stride = sizeof(uint32_t);
                    break;
                case TINYGLTF_COMPONENT_TYPE_SHORT:
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    stride = sizeof(uint16_t);
                    break;
                case TINYGLTF_COMPONENT_TYPE_BYTE:
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    stride = sizeof(uint8_t);
                    break;
                }
                switch(accessor.type) {
                case TINYGLTF_TYPE_VEC2:
                    stride *= 2;
                    break;
                case TINYGLTF_TYPE_VEC3:
                    stride *= 3;
                    break;
                case TINYGLTF_TYPE_VEC4:
                    stride *= 4;
                    break;
                case TINYGLTF_TYPE_MAT2:
                    stride *= 4;
                    break;
                case TINYGLTF_TYPE_MAT3:
                    stride *= 9;
                    break;
                case TINYGLTF_TYPE_MAT4:
                    stride *= 16;
                    break;
                default:
                    MCH_WARN("Unsupported accessor.type {} {} {}", accessor.type, __FILE__, __LINE__)
                }
            }));
            if (scene.attribute_datas.find(attribute_name) == scene.attribute_datas.end()) {
                scene.attribute_datas[attribute_name] = {};
            }
            auto &data = scene.attribute_datas[attribute_name];
            auto start = data.size();
            data.resize(start + count * stride);
            MCH_DEBUG("{}: count {}, stride {}", attribute_name, count, stride)
            memcpy(data.data() + start, ptr, count * stride);
        }

        const auto &index_accessor = gltf_model.accessors[gltf_primitive.indices];
        const auto &index_buffer_view = gltf_model.bufferViews[index_accessor.bufferView];
        index_count = index_accessor.count;
        auto *index_ptr = static_cast<const uint8_t *>(gltf_model.buffers[index_buffer_view.buffer].data.data() + index_accessor.byteOffset + index_buffer_view.byteOffset);
        switch (index_accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            for (uint32_t i = 0; i < index_count; i ++) {
                scene.indices.push_back(*(reinterpret_cast<const uint32_t *>(index_ptr)));
                index_ptr += 4;
            }
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            for (uint32_t i = 0; i < index_count; i ++) {
                scene.indices.push_back(*(reinterpret_cast<const uint16_t *>(index_ptr)));
                index_ptr += 2;
            }
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            for (uint32_t i = 0; i < index_count; i ++) {
                scene.indices.push_back(*(reinterpret_cast<const uint8_t *>(index_ptr)));
                index_ptr += 1;
            }
            break;
        }
    }

    GLTFPrimitive::~GLTFPrimitive() {
    }
}
