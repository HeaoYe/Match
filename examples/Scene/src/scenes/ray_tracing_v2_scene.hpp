#pragma once

#include "scene.hpp"
#include "../camera.hpp"

struct Material {
    glm::vec3 albedo;
    float roughness = 0;
    glm::vec3 spec_albedo = { 1, 1, 1 };
    float spec_prob = 0.2;
    glm::vec3 light_color = { 1, 1, 1 };
    float light_intensity = 0;
};

class RayTracingV2Scene : public Scene {
    define_scene(RayTracingV2Scene)
private:
    int frame_count = 0;
    std::unique_ptr<Camera> camera;
    
    
    Material dragon_material {};
    std::shared_ptr<Match::Model> model;
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
