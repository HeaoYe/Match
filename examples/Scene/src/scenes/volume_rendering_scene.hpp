#pragma once

#include "scene.hpp"
#include "../camera.hpp"

enum class SphereType : int {
    eGround = 0,  // 大地
    eUniform = 1,  // 均匀介质
    ePerlin = 2,  // 柏林噪音
    eCloud = 3,  // 云（数据由迪士尼提供）
    eSmoke = 4,  // 烟（数据由openVDB提供）
};

struct VolumeRenderingArgs {
    // Uniform
    float sigma_maj_uniform = 0.8;
    float sigma_a_uniform = 0.5;
    float sigma_s_uniform = 0.2;
    float g_uniform = 0;

    // Perlin
    float sigma_maj_perlin = 7.8;
    float sigma_a_perlin = 5;
    float sigma_s_perlin = 0.16;
    float g_perlin = 0;
    float L = 3.376;
    float H = 0.728;
    float freq = 0.506;
    int OCT = 3;

    // Cloud
    float sigma_maj_cloud = 10;
    float sigma_a_cloud = 0.4;
    float sigma_s_cloud = 9.4;
    float g_cloud = -0.9;

    // Smoke
    float sigma_maj_smoke = 12;
    float sigma_a_smoke = 9.59;
    float sigma_s_smoke = 1.76;
    float g_smoke = -0.9;
};

class VolumeRenderingScene : public Scene {
    define_scene(VolumeRenderingScene);
private:
    int frame_count = 0;
    std::unique_ptr<Camera> camera;

    std::shared_ptr<Match::SphereCollect> sphere_collect;
    std::shared_ptr<Match::UniformBuffer> args;
    std::shared_ptr<Match::VolumeData> volume_data_cloud;
    std::shared_ptr<Match::TwoStageBuffer> volume_buffer_cloud;
    std::shared_ptr<Match::VolumeData> volume_data_smoke;
    std::shared_ptr<Match::TwoStageBuffer> volume_buffer_smoke;

    std::shared_ptr<Match::StorageImage> ray_tracing_result_image;
    std::shared_ptr<Match::RayTracingInstanceCollect> instance_collect;

    std::shared_ptr<Match::PushConstants> ray_tracing_shader_program_constants;
    std::shared_ptr<Match::DescriptorSet> ray_tracing_shader_program_ds;
    std::shared_ptr<Match::RayTracingShaderProgram> ray_tracing_shader_program;

    std::shared_ptr<Match::Sampler> sampelr;
    std::shared_ptr<Match::DescriptorSet> shader_program_ds;
    std::shared_ptr<Match::GraphicsShaderProgram> shader_program;
};
