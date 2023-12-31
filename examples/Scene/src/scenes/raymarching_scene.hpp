#pragma once

#include "scene.hpp"
#include "../camera.hpp"

class RayMarchingScene : public Scene {
    define_scene(RayMarchingScene)
    struct RayMarchingUniform {
        alignas(16) glm::vec2 window_size;
        alignas(4) int max_steps;
        alignas(4) float max_dist;
        alignas(4) float epsillon_dist;
        alignas(16) glm::vec4 sphere;
        alignas(16) glm::vec3 light;
    } *data;
private:
    std::unique_ptr<Camera> camera;
    std::shared_ptr<Match::UniformBuffer> uniform_buffer;
    std::shared_ptr<Match::ShaderProgram> shader_program;
};
