#pragma once

#include <Match/Match.hpp>
#include <tiny_gltf.h>
#include <glm/gtc/quaternion.hpp>

extern Match::APIManager *ctx;
extern std::shared_ptr<Match::ResourceFactory> factory;
class GLTFModel;

struct GLTFVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 color;
    glm::vec4 joint0;
    glm::vec4 weight0;
    glm::vec4 tangent;
};

class GLTFTexture {
    no_copy_move_construction(GLTFTexture)
    friend GLTFModel;
public:
    GLTFTexture(GLTFModel &model, const tinygltf::Image &image);
private:
    bool is_ktx;
    uint32_t index;
    std::shared_ptr<Match::Texture> texture;
};

class GLTFMaterial {
    default_no_copy_move_construction(GLTFMaterial)
public:
    enum class AlphaMode {
        eOpaque,
        eMask,
        eBlend,
    };
    AlphaMode alpha_mode = AlphaMode::eOpaque;
    float alpha_cutoff = 1.0f;
    float metallic_factor = 1.0f;
    float roughness_factor = 1.0f;
    glm::vec4 base_color_factor = glm::vec4(1.0f);
    std::shared_ptr<GLTFTexture> base_color_texture = nullptr;
    std::shared_ptr<GLTFTexture> metallic_roughness_texture = nullptr;
    std::shared_ptr<GLTFTexture> normal_texture = nullptr;
    std::shared_ptr<GLTFTexture> occlusion_texture = nullptr;
    std::shared_ptr<GLTFTexture> emissive_texture = nullptr;

    std::shared_ptr<GLTFTexture> specular_glossiness_texture = nullptr;
    std::shared_ptr<GLTFTexture> diffuse_texture = nullptr;

    vk::DescriptorSet descriptorSet {};
public:
    void createDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorBindingFlags);
};

class GLTFNode;

struct GLTFDimensions {
    glm::vec3 min = glm::vec3(FLT_MAX);
    glm::vec3 max = glm::vec3(-FLT_MAX);
    glm::vec3 size;
    glm::vec3 center;
    float radius;

    void update() {
        size = max - min;
        center = (min + max) / 2.0f;
        radius = glm::distance(min, max) / 2.0f;
    }
};

class GLTFPrimitive {
    no_copy_move_construction(GLTFPrimitive)
public:
    uint32_t first_index;
    uint32_t index_count;
    uint32_t first_vertex;
    uint32_t vertex_count;
    std::shared_ptr<GLTFMaterial> material;

    GLTFDimensions dimensions;

    void setDimensions(glm::vec3 min, glm::vec3 max) {
        dimensions.min = min;
        dimensions.max = max;
        dimensions.update();
    }

    GLTFPrimitive(uint32_t first_index, uint32_t index_count, std::shared_ptr<GLTFMaterial> material) : first_index(first_index), index_count(index_count), material(material) {}
};

class GLTFMesh {
    no_copy_move_construction(GLTFMesh)
public:
    std::vector<std::unique_ptr<GLTFPrimitive>> primitives;
    std::string name;

    std::shared_ptr<Match::UniformBuffer> uniform_buffer;

    struct UniformBlock {
        glm::mat4 matrix;
        glm::mat4 joint_matrix[64] {};
        float joint_count { 0 };
    } uniform_block;

    GLTFMesh(glm::mat4 matrix) {
        uniform_block.matrix = matrix;
        uniform_buffer = factory->create_uniform_buffer(sizeof(UniformBlock));
    }

    ~GLTFMesh() {
        primitives.clear();
    }
};

class GLTFSkin {
    default_no_copy_move_construction(GLTFSkin)
public:
    std::string name;
    GLTFNode* skeleton_root = nullptr;
    std::vector<glm::mat4> inverse_bind_matrices;
    std::vector<GLTFNode*> joints;
};

class GLTFNode {
    default_no_copy_move_construction(GLTFNode)
public:
    GLTFNode* parent;
    uint32_t index;
    std::vector<std::unique_ptr<GLTFNode>> children;
    glm::mat4 matrix;
    std::string name;
    std::unique_ptr<GLTFMesh> mesh;
    GLTFSkin *skin;
    int32_t skin_index = -1;
    glm::vec3 translation;
    glm::vec3 scale;
    glm::quat rotation;

    glm::mat4 local_matrix() {
        return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
    }

    glm::mat4 get_matrix() {
        auto m = local_matrix();
        auto *node = parent;
        while (node) {
            m = node->local_matrix() * m;
            node = node->parent;
        }
        return m;
    }

    void update();

    ~GLTFNode() {
        mesh.reset();
        children.clear();
    }
};

class GLTFModel {
    no_copy_move_construction(GLTFModel)
    friend GLTFTexture;
public:
    GLTFModel(const std::string &filename, float scale = 1.0f);
    ~GLTFModel();
private:
    void load_images(const tinygltf::Model &model);
    void load_materials(const tinygltf::Model &model);
    void load_node(const tinygltf::Model &model, GLTFNode *parent, const tinygltf::Node &node, uint32_t node_index, std::vector<GLTFVertex> &vertices, std::vector<uint32_t> &indices, float global_scale);
    void load_skins(const tinygltf::Model &model);
    GLTFNode *get_node(uint32_t index) {
        for (auto *node : all_node_references) {
            if (node->index == index) {
                return node;
            }
        }
        return nullptr;
    }
public:
    bool metallic_roughness_workflow = true;
    // bool buffersBound = false;
    std::string path;

    GLTFDimensions dimensions;

    std::vector<std::shared_ptr<GLTFTexture>> textures;
    std::shared_ptr<Match::Texture> empty_texture;
    std::vector<std::shared_ptr<GLTFMaterial>> materials;
    std::vector<std::unique_ptr<GLTFNode>> nodes;
    std::vector<GLTFNode*> all_node_references;
    std::vector<std::unique_ptr<GLTFSkin>> skins;
    std::shared_ptr<Match::VertexBuffer> vertex_buffer;
    std::shared_ptr<Match::IndexBuffer> index_buffer;
};
