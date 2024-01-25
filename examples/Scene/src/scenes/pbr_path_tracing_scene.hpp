#pragma once

#include "scene.hpp"
#include "../camera.hpp"

class PBRPathTracingScene : public Scene {
    define_scene(PBRPathTracingScene)
private:
    int frame_count = 0;
    std::unique_ptr<Camera> camera;
    
    std::shared_ptr<Match::GLTFScene> gltf_scene;

    std::shared_ptr<Match::StorageImage> ray_tracing_result_image;
    std::shared_ptr<Match::RayTracingInstanceCollect> instance_collect;

    std::shared_ptr<Match::PushConstants> ray_tracing_shader_program_constants;
    std::shared_ptr<Match::DescriptorSet> ray_tracing_shader_program_ds;
    std::shared_ptr<Match::RayTracingShaderProgram> ray_tracing_shader_program;

    std::shared_ptr<Match::Sampler> sampelr;
    std::shared_ptr<Match::DescriptorSet> shader_program_ds;
    std::shared_ptr<Match::GraphicsShaderProgram> shader_program;
};
