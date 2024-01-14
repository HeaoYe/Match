#pragma once

#include "scene.hpp"
#include "../camera.hpp"

struct SphereMaterial {
    glm::vec3 albedo;
    float roughness = 0;
    glm::vec3 light_color = { 1, 1, 1 };
    float light_intensity = 0;
};

class RayTracingV2Scene : public Scene {
    define_scene(RayTracingV2Scene)
private:
    std::unique_ptr<Camera> camera;
    
    std::shared_ptr<Match::SphereCollect> sphere_collect;

    std::shared_ptr<Match::StorageImage> ray_tracing_result_image;
    std::shared_ptr<Match::RayTracingInstanceCollect> instance_collect;

    std::shared_ptr<Match::PushConstants> ray_tracing_shader_program_constants;
    std::shared_ptr<Match::DescriptorSet> ray_tracing_shader_program_ds;
    std::shared_ptr<Match::RayTracingShaderProgram> ray_tracing_shader_program;

    std::shared_ptr<Match::Sampler> sampelr;
    std::shared_ptr<Match::DescriptorSet> shader_program_ds;
    std::shared_ptr<Match::GraphicsShaderProgram> shader_program;
};
