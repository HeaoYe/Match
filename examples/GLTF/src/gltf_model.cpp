#include "gltf_model.hpp"
#include <Match/vulkan/descriptor_resource/spec_texture.hpp>
#include <glm/gtc/type_ptr.hpp>

GLTFModel::GLTFModel(const std::string &filename, float scale) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;

    size_t pos = filename.find_last_of('/');
	path = filename.substr(0, pos);
    
    auto ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
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

    load_images(model);
    load_materials(model);

    std::vector<GLTFVertex> vertices;
    std::vector<uint32_t> indices;

    auto &scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
    for (auto node_index : scene.nodes) {
        load_node(model, nullptr, model.nodes[scene.nodes[node_index]], node_index, vertices, indices, scale);
    }

    load_skins(model);
    
    for (auto node : all_node_references) {
        if (node->skin_index > -1) {
            MCH_WARN("UNIMPL SKIN")
            node->skin = skins[node->skin_index].get();
        }
        if (node->mesh) {
            node->update();
        }
    }

    for (auto extension : model.extensionsUsed) {
        if (extension == "KHR_materials_pbrSpecularGlossiness") {
            MCH_INFO("Required exrension", extension)
            metallic_roughness_workflow = false;
        }
    }

    vertex_buffer = factory->create_vertex_buffer(sizeof(GLTFVertex), vertices.size());
    vertex_buffer->upload_data_from_vector(vertices);
    index_buffer = factory->create_index_buffer(Match::IndexType::eUint32, indices.size());
    index_buffer->upload_data_from_vector(indices);

    dimensions.min = glm::vec3(-FLT_MAX);
    dimensions.max = glm::vec3(FLT_MAX);
    std::function<void(GLTFNode *, glm::vec3 &, glm::vec3 &)> get_node_dimensions = [&](auto *node, auto &min, auto &max) {
        if (node->mesh) {
            for (auto &primitive : node->mesh->primitives) {
                auto loc_min = glm::vec4(primitive->dimensions.min, 1.0f) * node->get_matrix();
                auto loc_max = glm::vec4(primitive->dimensions.max, 1.0f) * node->get_matrix();
                if (loc_min.x < min.x) { min.x = loc_min.x; }
                if (loc_min.y < min.y) { min.y = loc_min.y; }
                if (loc_min.z < min.z) { min.z = loc_min.z; }
                if (loc_max.x > max.x) { max.x = loc_max.x; }
                if (loc_max.y > max.y) { max.y = loc_max.y; }
                if (loc_max.z > max.z) { max.z = loc_max.z; }
            }
        }
        for (auto &child : node->children) {
            get_node_dimensions(child.get(), min, max);
        }
    };
    for (auto &node : nodes) {
        get_node_dimensions(node.get(), dimensions.min, dimensions.max);
    }
    dimensions.update();
}

GLTFTexture::GLTFTexture(GLTFModel &model, const tinygltf::Image &image) : is_ktx(false), index(-1u) {
    if (image.uri.empty()) {
        MCH_WARN("Unknown GLTF Image")
        return;
    }

    std::string::size_type pos;
    if ((pos = image.uri.find_last_of('.')) != std::string::npos) {
        if (image.uri.substr(pos + 1) == "ktx") {
            is_ktx = true;
        }
    }

    if (!is_ktx) {
        std::vector<uint8_t> rgba;
        const uint8_t *buffer_readonly = nullptr;
        vk::DeviceSize buffer_size = 0;
        if (image.component == 3) {
            buffer_size = image.width * image.height * 4;
            rgba.resize(buffer_size);
            auto *rgb = image.image.data();
            auto *ptr = rgba.data();
            for (uint32_t i = 0; i < image.width * image.height; i ++) {
                ptr[0] = rgb[0];
                ptr[1] = rgb[1];
                ptr[2] = rgb[2];
                ptr[3] = 0;
                ptr += 4;
                rgb += 3;
            }
            buffer_readonly = rgba.data();
            MCH_INFO("Convert RGB -> RGBA {}", buffer_size)
        } else {
            assert(image.component == 4);
            buffer_readonly = image.image.data();
            buffer_size = image.image.size();
            MCH_INFO("Load RGBA {}", buffer_size)
        }
        texture = factory->create_texture(buffer_readonly, image.width, image.height);
    } else {
        texture = factory->load_texture(model.path + "/" + image.uri);
    }
}

void GLTFModel::load_images(const tinygltf::Model &model) {
    for (auto &image : model.images) {
        auto &texture = textures.emplace_back(std::make_shared<GLTFTexture>(*this, image));
        texture->index = textures.size() - 1;
    }
    uint8_t empty_data[] = { 0, 0, 0, 0 };
    empty_texture = factory->create_texture(empty_data, 1, 1, 1);
}

#define get_texture(v, name, member) \
if (mat.v.find("name") != mat.v.end()) { \
    material->member = this->textures[model.textures.at(mat.v.at("name").TextureIndex()).source]; \
} \

#define get_factor(v, name, member) \
if (mat.v.find("name") != mat.v.end()) { \
    material->member = static_cast<float>(mat.v.at("name").Factor()); \
} \

#define get_color_factor(v, name, member) \
if (mat.v.find("name") != mat.v.end()) { \
    material->member = glm::make_vec4(mat.v.at("name").ColorFactor().data()); \
} \

void GLTFModel::load_materials(const tinygltf::Model &model) {
    for (const auto &mat : model.materials) {
        auto &material = materials.emplace_back(std::make_shared<GLTFMaterial>());

        get_texture(values, baseColorTexture, base_color_texture);
        get_texture(values, metallicRoughnessTexture, metallic_roughness_texture);
        get_factor(values, roughnessFactor, roughness_factor);
        get_factor(values, metallicFactor, metallic_factor);
        get_color_factor(values, baseColorFactor, base_color_factor);
        get_texture(additionalValues, normalTexture, normal_texture);
        get_texture(additionalValues, emissiveTexture, emissive_texture);
        get_texture(additionalValues, occlusionTexture, occlusion_texture);
        if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
			auto &param = mat.additionalValues.at("alphaMode");
			if (param.string_value == "BLEND") {
				material->alpha_mode = GLTFMaterial::AlphaMode::eBlend;
			}
			if (param.string_value == "MASK") {
				material->alpha_mode = GLTFMaterial::AlphaMode::eMask;
			}
		}
        get_factor(additionalValues, alphaCutoff, alpha_cutoff);
    }
}

#undef get_color_factor
#undef get_factor
#undef get_texture

void GLTFModel::load_node(const tinygltf::Model &model, GLTFNode *parent, const tinygltf::Node &node, uint32_t node_index, std::vector<GLTFVertex> &vertices, std::vector<uint32_t> &indices, float global_scale) {
    auto new_node = std::make_unique<GLTFNode>();
    new_node->index = node_index;
    new_node->parent = parent;
    new_node->name = node.name;
    new_node->skin_index = node.skin;
    new_node->matrix = glm::mat4(1.0f);

    if (node.translation.size() == 3) {
        new_node->translation = glm::make_vec3(node.translation.data());
    } else {
        new_node->translation = glm::vec3(0);
    }

    if (node.rotation.size() == 4) {
        glm::quat q = glm::make_quat(node.rotation.data());
        new_node->rotation = glm::mat4(q);
    } else {
        new_node->rotation = glm::mat4(1);
    }

    if (node.scale.size() == 3) {
        new_node->scale = glm::make_vec3(node.scale.data());
    } else {
        new_node->scale = glm::vec3(1.0f);
    }

    if (node.matrix.size() == 16) {
        new_node->matrix = glm::make_mat4x4(node.matrix.data());
        glm::scale(new_node->matrix, glm::vec3(global_scale));
    }

    if (node.children.size() > 0) {
        for (auto child_node_index : node.children) {
            load_node(model, new_node.get(), model.nodes[child_node_index], child_node_index, vertices, indices, global_scale);
        }
    }

    if (node.mesh > -1) {
        auto &mesh = model.meshes[node.mesh];
        new_node->mesh = std::make_unique<GLTFMesh>(new_node->matrix);
        new_node->mesh->name = mesh.name;
        for (auto &primitive : mesh.primitives) {
            if (primitive.indices < 0) {
                continue;
            }
            uint32_t vertex_start = vertices.size();
            uint32_t index_start = indices.size();
            uint32_t vertex_count = 0;
            uint32_t index_count = 0;
            glm::vec3 pos_min {}, pos_max {};
            bool has_skin = false;

            const float *buffer_pos = nullptr;
            const float *buffer_normals = nullptr;
            const float *buffer_tex_coords = nullptr;
            const float *buffer_colors = nullptr;
            uint32_t num_color_components;
            const float *buffer_tangents = nullptr;
            const uint16_t *buffer_joints = nullptr;
            const float *buffer_weights = nullptr;

            assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

            auto no_operator = [](const tinygltf::Accessor &) {};
            auto get_data_pointer = [&](const std::string &name, auto other_operator) {
                if (primitive.attributes.find(name) == primitive.attributes.end()) {
                    return static_cast<const void *>(nullptr);
                }
                const auto &accessor = model.accessors[primitive.attributes.find(name)->second];
                const auto &buffer_view = model.bufferViews[accessor.bufferView];
                other_operator(accessor);
                return reinterpret_cast<const void *>(model.buffers[buffer_view.buffer].data.data() + accessor.byteOffset + buffer_view.byteOffset);
            };

            buffer_pos = static_cast<const float *>(get_data_pointer("POSITION", [&](const auto &accessor) {
                pos_min = glm::make_vec3(accessor.minValues.data());
                pos_max = glm::make_vec3(accessor.maxValues.data());
                vertex_count = accessor.count;
            }));
            buffer_normals = static_cast<const float *>(get_data_pointer("NORMAL", no_operator));
            buffer_tex_coords = static_cast<const float *>(get_data_pointer("TEXCOORD_0", no_operator));
            buffer_colors = static_cast<const float *>(get_data_pointer("COLOR_0", [&](const auto &accessor) {
                num_color_components = accessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
            }));
            buffer_tangents = static_cast<const float *>(get_data_pointer("TANGENT", no_operator));
            buffer_joints = static_cast<const uint16_t *>(get_data_pointer("JOINTS_0", no_operator));
            buffer_weights = static_cast<const float *>(get_data_pointer("WEIGHTS_0", no_operator));
            has_skin = (buffer_joints && buffer_weights);

            for (size_t v = 0; v < vertex_count; v ++) {
                auto &vertex = vertices.emplace_back();
                vertex.pos = glm::make_vec3(&buffer_pos[v * 3]);
                vertex.normal = glm::normalize(glm::vec3(buffer_normals ? glm::make_vec3(&buffer_normals[v * 3]) : glm::vec3(0)));
                vertex.uv = buffer_tex_coords ? glm::make_vec2(&buffer_tex_coords[v * 2]) : glm::vec2(0);
                if (buffer_colors) {
                    switch (num_color_components) {
                    case 3:
                        vertex.color = glm::vec4(glm::make_vec3(&buffer_colors[v * 3]), 1);
                        break;
                    case 4:
                        vertex.color = glm::make_vec4(&buffer_colors[v * 4]);
                        break;
                    }
                } else {
                    vertex.color = glm::vec4(1);
                }
                vertex.tangent = buffer_tangents ? glm::make_vec4(&buffer_tangents[v * 4]) : glm::vec4(0);
                vertex.joint0 = has_skin ? glm::vec4(glm::make_vec4(&buffer_joints[v * 4])) : glm::vec4(0);
                vertex.weight0 = has_skin ? glm::make_vec4(&buffer_weights[v * 4]) : glm::vec4(0);
            }

            const auto &accessor = model.accessors[primitive.indices];
            const auto &buffer_view = model.bufferViews[accessor.bufferView];
            const auto *buffer_indices = model.buffers[buffer_view.buffer].data.data() + accessor.byteOffset + buffer_view.byteOffset;
            index_count = accessor.count;

            for (uint32_t i = 0; i < index_count; i ++) {
                switch (accessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                    indices.push_back(*reinterpret_cast<const uint32_t *>(buffer_indices));
                    buffer_indices += 4;
                    break;
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                    indices.push_back(*reinterpret_cast<const uint16_t *>(buffer_indices));
                    buffer_indices += 2;
                    break;
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                    indices.push_back(*reinterpret_cast<const uint8_t *>(buffer_indices));
                    buffer_indices += 1;
                    break;
                default:
                    MCH_ERROR("Unknown index type {}", accessor.componentType)
                }
            }

            auto &new_primitive = new_node->mesh->primitives.emplace_back(std::make_unique<GLTFPrimitive>(index_start, index_count, primitive.material > -1 ? materials[primitive.material] : materials.back()));
            new_primitive->first_vertex = vertex_start;
            new_primitive->vertex_count = vertex_count;
            new_primitive->setDimensions(pos_min, pos_max);
            MCH_INFO("ADD PRIMITIVE: HS {} BP {} BN {} BTC {} BC {} BT {} BJ {} BW {}", has_skin, (uint64_t)buffer_pos, (uint64_t)buffer_normals, (uint64_t)buffer_tex_coords, (uint64_t)buffer_colors, (uint64_t)buffer_tangents, (uint64_t)buffer_joints, (uint64_t)buffer_weights)
            MCH_INFO("ADD PRIMITIVE: vertices {} indices {}", vertex_count, index_count)
        }
    }

    all_node_references.push_back(new_node.get());
    if (parent) {
        parent->children.push_back(std::move(new_node));
    } else {
        nodes.push_back(std::move(new_node));
    }
}

void GLTFNode::update() {
	if (mesh) {
		auto mat = get_matrix();
		if (skin) {
			mesh->uniform_block.matrix = mat;
			glm::mat4 inverseTransform = glm::inverse(mat);
			for (size_t i = 0; i < skin->joints.size(); i++) {
				auto *joint_node = skin->joints[i];
				glm::mat4 joint_mat = joint_node->get_matrix() * skin->inverse_bind_matrices[i];
				joint_mat = inverseTransform * joint_mat;
				mesh->uniform_block.joint_matrix[i] = joint_mat;
			}
			mesh->uniform_block.joint_count = (float)skin->joints.size();
			memcpy(mesh->uniform_buffer->get_uniform_ptr(), &mesh->uniform_block, sizeof(GLTFMesh::UniformBlock));
		} else {
			memcpy(mesh->uniform_buffer->get_uniform_ptr(), &mat, sizeof(glm::mat4));
		}
	}

    MCH_INFO("Update self And {} children", children.size())

	for (auto& child : children) {
		child->update();
	}
}

void GLTFModel::load_skins(const tinygltf::Model &model) {
    for (const auto &skin : model.skins) {
        auto new_skin = std::make_unique<GLTFSkin>();
        new_skin->name = skin.name;

        if (skin.skeleton > -1) {
            new_skin->skeleton_root = get_node(skin.skeleton);
            assert(new_skin->skeleton_root != nullptr);
        }

        for (auto joint_index : skin.joints) {
            auto *node = get_node(joint_index);
            if (node) {
                new_skin->joints.push_back(node);
            }
        }

        if (skin.inverseBindMatrices > -1) {
            const auto &accessor = model.accessors[skin.inverseBindMatrices];
			const auto &view = model.bufferViews[accessor.bufferView];
			const auto &buffer = model.buffers[view.buffer];
			new_skin->inverse_bind_matrices.resize(accessor.count);
			memcpy(new_skin->inverse_bind_matrices.data(), (buffer.data.data() + accessor.byteOffset + view.byteOffset), accessor.count * sizeof(glm::mat4));
        }

        skins.push_back(std::move(new_skin));
    }
}

GLTFModel::~GLTFModel() {
    vertex_buffer.reset();
    index_buffer.reset();
    skins.clear();
    all_node_references.clear();
    nodes.clear();
    materials.clear();
    empty_texture.reset();
    textures.clear();
}
