#pragma once

#include "scene.hpp"
#include "../camera.hpp"


class PBRMaterial {
public:
    PBRMaterial(Match::ResourceFactory &factory);
public:
    struct PBRMaterialUniform {
        alignas(16) glm::vec3 color;    // 颜色
        alignas(4) float roughness;     // 粗糙度
        alignas(4) float metallic;      // 金属度
        alignas(4) float reflectance;   // 反射度
    } *data;
    std::shared_ptr<Match::UniformBuffer> uniform_buffer;
};

class Lights {
public:
    Lights(Match::ResourceFactory &factory);
public:
    struct PointLight {
        alignas(16) glm::vec3 pos;
        alignas(16) glm::vec3 color;
    };
    struct LightsUniform {
        alignas(16) glm::vec3 direction;            // 平行光
        alignas(16) glm::vec3 color;                // 平行光颜色
        alignas(16) PointLight point_lights[3];     // 点光源
    } *data;
    std::shared_ptr<Match::UniformBuffer> uniform_buffer;
};

class PBRScene : public Scene {
    define_scene(PBRScene)
private:
    std::unique_ptr<Camera> camera;
    std::unique_ptr<PBRMaterial> material;
    std::unique_ptr<Lights> lights;
    std::shared_ptr<Match::ShaderProgram> shader_program;
    std::shared_ptr<Match::Model> model;
    std::shared_ptr<Match::VertexBuffer> vertex_buffer;
    std::shared_ptr<Match::IndexBuffer> index_buffer;
};
