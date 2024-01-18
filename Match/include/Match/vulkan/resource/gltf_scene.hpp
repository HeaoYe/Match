#pragma once

#include <Match/vulkan/descriptor_resource/texture.hpp>
#include <Match/vulkan/resource/model.hpp>
#include <glm/gtc/quaternion.hpp>
#include <tiny_gltf.h>

namespace Match {
    class GLTFScene;

    class GLTFPrimitive {
        default_no_copy_move_construction(GLTFPrimitive)
    public:
        GLTFPrimitive(GLTFScene &scene, const tinygltf::Model &gltf_model, const tinygltf::Primitive &gltf_primitive);
        ~GLTFPrimitive();
    INNER_VISIBLE:
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
    INNER_VISIBLE:
        std::string name {};
        std::vector<std::unique_ptr<GLTFPrimitive>> primitives {};
    };

    class GLTFScene : public RayTracingModel {
        no_copy_move_construction(GLTFScene)
    public:
        GLTFScene(const std::string &filename);
        ~GLTFScene();
        RayTracingModelType get_ray_tracing_mode_type() override { return RayTracingModel::RayTracingModelType::eGLTFScene; }
    INNER_VISIBLE:
        void load_images(const tinygltf::Model &gltf_model);
    INNER_VISIBLE:
        std::string path;
        std::vector<std::shared_ptr<Texture>> textures;
        std::shared_ptr<Texture> empty_texture;
        std::vector<std::shared_ptr<GLTFMesh>> meshes;
        std::vector<glm::vec3> positions;
        std::vector<uint32_t> indices;
    };
}
