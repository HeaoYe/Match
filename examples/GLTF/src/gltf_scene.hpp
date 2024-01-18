#pragma once

#include <Match/Match.hpp>
#include <tiny_gltf.h>
#include <glm/gtc/quaternion.hpp>

extern Match::APIManager *ctx;
extern std::shared_ptr<Match::ResourceFactory> factory;

class GLTFScene;

class GLTFPrimitive {
    default_no_copy_move_construction(GLTFPrimitive)
public:
    GLTFPrimitive(GLTFScene &scene, const tinygltf::Model &gltf_model, const tinygltf::Primitive &gltf_primitive);
    ~GLTFPrimitive();

    uint32_t first_index { 0 };
    uint32_t index_count { 0 };
    uint32_t first_vertex { 0 };
    uint32_t vertex_count { 0 };
    uint32_t material_index { 0 };

    glm::vec3 pos_min { 0 };
    glm::vec3 pos_max { 0 };
};

class GLTFMesh {
    no_copy_move_construction(GLTFMesh)
public:
    GLTFMesh(GLTFScene &scene, const tinygltf::Model &gltf_model, const tinygltf::Mesh &gltf_mesh);
    ~GLTFMesh();

    std::string name {};
    std::vector<std::unique_ptr<GLTFPrimitive>> primitives {};
};

class GLTFNode {
    no_copy_move_construction(GLTFNode)
public:
    GLTFNode *parent { nullptr };
    std::vector<std::unique_ptr<GLTFNode>> children {};
    std::shared_ptr<GLTFMesh> mesh {};

    glm::vec3 translation { 1 };
    glm::vec3 scale { 1 };
    glm::quat rotation {};
};

class GLTFScene {
    no_copy_move_construction(GLTFScene)
public:
    std::string path;
    std::vector<std::shared_ptr<Match::Texture>> textures;
    std::shared_ptr<Match::Texture> empty_texture;
    // std::vector<std::shared_ptr<GLTFMaterial>> materials;
    std::vector<std::shared_ptr<GLTFMesh>> meshes;
    std::vector<glm::vec3> positions;
    std::vector<uint32_t> indices;
public:
    GLTFScene(const std::string &filename);
    ~GLTFScene();
    void load_images(const tinygltf::Model &gltf_model);
    void load_materials(const tinygltf::Model &gltf_model);
    void load_node(const tinygltf::Model &gltf_model, tinygltf::Node *node);
};
