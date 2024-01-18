#include <Match/vulkan/resource/gltf_scene.hpp>
#include <Match/vulkan/descriptor_resource/spec_texture.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Match {
    GLTFScene::GLTFScene(const std::string &filename) {
        tinygltf::TinyGLTF loader;
        tinygltf::Model gltf_model;
        std::string err, warn;

        size_t pos = filename.find_last_of('/');
        path = filename.substr(0, pos);
        
        auto ret = loader.LoadASCIIFromFile(&gltf_model, &err, &warn, filename);
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
        
        for (auto &gltf_mesh : gltf_model.meshes) {
            meshes.push_back(std::make_shared<GLTFMesh>(*this, gltf_model, gltf_mesh));
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
                    textures.push_back(std::make_shared<Match::KtxTexture>(path + "/" + gltf_image.uri));
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
        
        uint8_t empty_data[] = { 0, 0, 0, 0 };
        empty_texture = std::make_shared<DataTexture>(empty_data, 1, 1, 1);
    }

    GLTFScene::~GLTFScene() {
        empty_texture.reset();
        textures.clear();
        meshes.clear();
        indices.clear();
        positions.clear();
    }

    GLTFMesh::GLTFMesh(GLTFScene &scene, const tinygltf::Model &gltf_model, const tinygltf::Mesh &gltf_mesh) {
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
            primitives.push_back(std::make_unique<GLTFPrimitive>(scene, gltf_model, gltf_primitive));
        }
    }

    GLTFMesh::~GLTFMesh() {
        primitives.clear();
    }

    GLTFPrimitive::GLTFPrimitive(GLTFScene &scene, const tinygltf::Model &gltf_model, const tinygltf::Primitive &gltf_primitive) {
        first_index = scene.indices.size();
        first_vertex = scene.positions.size();
        material_index = gltf_primitive.material;

        auto no_operator = [](const tinygltf::Accessor &) {};
        auto get_data_pointer = [&](const std::string &name, auto other_operator) {
            if (gltf_primitive.attributes.find(name) == gltf_primitive.attributes.end()) {
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

        const auto &index_accessor = gltf_model.accessors[gltf_primitive.indices];
        const auto &index_buffer_view = gltf_model.bufferViews[index_accessor.bufferView];
        index_count = index_accessor.count;
        auto *index_ptr = static_cast<const uint8_t *>(gltf_model.buffers[index_buffer_view.buffer].data.data() + index_accessor.byteOffset + index_buffer_view.byteOffset);
        for (uint32_t i = 0; i < index_count; i ++) {
            switch (index_accessor.componentType) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                scene.indices.push_back(*(reinterpret_cast<const uint32_t *>(index_ptr)) + first_vertex);
                index_ptr += 4;
                break;
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                scene.indices.push_back(*(reinterpret_cast<const uint16_t *>(index_ptr)) + first_vertex);
                index_ptr += 2;
                break;
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                scene.indices.push_back(*(reinterpret_cast<const uint8_t *>(index_ptr)) + first_vertex);
                index_ptr += 1;
                break;
            }
        }
        first_index = 0;
        first_vertex = 0;
    }

    GLTFPrimitive::~GLTFPrimitive() {
    }
}