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
    // 反照率
    glm::vec3 albedo;
    // 类型
    Type type;

    // 折射率
    float IOR;
    // 导体的折射率为复数，IOR为实部，K为虚部
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

    MicrofacetTheoryMaterial bunny_d_material {};
    MicrofacetTheoryMaterial bunny_dt_material {};
    MicrofacetTheoryMaterial bunny_c_material {};
    std::shared_ptr<Match::Model> model_bunny;

    MicrofacetTheoryMaterial outter_d_material {};
    MicrofacetTheoryMaterial outter_c_material {};
    MicrofacetTheoryMaterial inner_material {};
    std::shared_ptr<Match::Model> model_outter;
    std::shared_ptr<Match::Model> model_inner;

    MicrofacetTheoryMaterial dragon_material {};
    std::shared_ptr<Match::Model> model_dragon;

    std::shared_ptr<Match::StorageImage> ray_tracing_result_image;
    std::shared_ptr<Match::RayTracingInstanceCollect> instance_collect;

    std::shared_ptr<Match::PushConstants> ray_tracing_shader_program_constants;
    std::shared_ptr<Match::DescriptorSet> ray_tracing_shader_program_ds;
    std::shared_ptr<Match::RayTracingShaderProgram> ray_tracing_shader_program;

    std::shared_ptr<Match::Sampler> sampelr;
    std::shared_ptr<Match::DescriptorSet> shader_program_ds;
    std::shared_ptr<Match::GraphicsShaderProgram> shader_program;
};
