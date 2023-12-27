#pragma once

#include <Match/Match.hpp>
#include <glm/glm.hpp>

class Camera {
public:
    Camera(Match::ResourceFactory &factory);
    void update(float dt);
    void upload_data();
public:
    struct CameraUniform {
        // float, int 等4字节对齐
        // vec2 8字节对齐
        // vec3和vec4要16字节对齐
        // mat 16字节对齐
        alignas(16) glm::vec3 pos { 0, 0, 0 };
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 project;
    } data;
    float yaw, pitch;
    float speed = 2, sensi = 7;
    bool is_capture_cursor = false;
    std::shared_ptr<Match::UniformBuffer> uniform;
};
