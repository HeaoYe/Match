#pragma once

#include "scene.hpp"
#include "../camera.hpp"

struct MyCustomData {
    glm::vec3 color;
};

class RayTracingScene : public Scene {
    define_scene(RayTracingScene)
private:
    std::unique_ptr<Camera> camera;
    // 模型
    std::shared_ptr<Match::Model> model1;
    std::shared_ptr<Match::Model> model2;

    // 光追资源
    //                        pos   rotate_vector start_angle scale
    std::vector<std::tuple<glm::vec3, glm::vec3,     float,   float>> instance_transform_infos;
    // RayTracingInstance改名为RayTracingInstanceCollect
    // Vulkan的顶层加速结构管理多个实例,所以叫RayTracingInstanceCollect更合适
    // std::shared_ptr<Match::RayTracingInstanceCollect> instance_collect;
    std::shared_ptr<Match::RayTracingInstanceCollectWithCustomInstanceInfo<MyCustomData>> instance_collect;
    std::shared_ptr<Match::StorageImage> ray_tracing_result_image;
    std::shared_ptr<Match::DescriptorSet> ray_tracing_shader_program_ds;
    std::shared_ptr<Match::PushConstants> ray_tracing_shader_program_constants;
    std::shared_ptr<Match::RayTracingShaderProgram> ray_tracing_shader_program;

    // 光栅化资源
    std::shared_ptr<Match::Sampler> sampler;
    std::shared_ptr<Match::DescriptorSet> shader_program_ds;
    std::shared_ptr<Match::GraphicsShaderProgram> shader_program;
};
