#pragma once

#include "scene.hpp"
#include "../camera.hpp"

struct MicrofacetTheoryMaterial {
    enum class Type : int {
        // 介电质
        eDielectric = 0,
        // 导体
        eConductor = 1,
    };

    // 反射率
    glm::vec3 albedo;
    // 类型
    Type type;

    // 折射率
    float IOR;
    // 导体折射率的虚部
    float K;

    // 水平方向粗糙度
    float alpha_x;
    // 垂直方向粗糙度
    float alpha_y;
};

class MicrofacetTheoryScene : public Scene {
    define_scene(MicrofacetTheoryScene)
private:
    int frame_count = 0;
    std::unique_ptr<Camera> camera;

    MicrofacetTheoryMaterial dielectric_material {};
    MicrofacetTheoryMaterial conductor_material {};
    std::shared_ptr<Match::SphereCollect> sphere_collect;
    MicrofacetTheoryMaterial bunny1_material {};
    MicrofacetTheoryMaterial bunny2_material {};
    MicrofacetTheoryMaterial dragon_material {};
    MicrofacetTheoryMaterial outter1_material {};
    MicrofacetTheoryMaterial inner1_material {};
    MicrofacetTheoryMaterial outter2_material {};
    MicrofacetTheoryMaterial inner2_material {};
    std::shared_ptr<Match::Model> model_bunny;
    std::shared_ptr<Match::Model> model_dragon;
    std::shared_ptr<Match::Model> model_outter;
    std::shared_ptr<Match::Model> model_inner;

    std::shared_ptr<Match::StorageImage> ray_tracing_result_image;
    std::shared_ptr<Match::RayTracingInstanceCollect> instance_collect;

    std::shared_ptr<Match::PushConstants> ray_tracing_shader_program_constants;
    std::shared_ptr<Match::DescriptorSet> ray_tracing_shader_program_ds;
    std::shared_ptr<Match::RayTracingShaderProgram> ray_tracing_shader_program;

    std::shared_ptr<Match::Sampler> sampelr;
    std::shared_ptr<Match::DescriptorSet> shader_program_ds;
    std::shared_ptr<Match::GraphicsShaderProgram> shader_program;
};
