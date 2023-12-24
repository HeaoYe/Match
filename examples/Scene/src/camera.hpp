#pragma once

#include <Match/Match.hpp>
#include <glm/glm.hpp>

class Camera {
public:
    Camera(std::weak_ptr<Match::ResourceFactory> factory);
    void update(float dt);
    void upload_data();
public:
    struct CameraUniform {
        alignas(4) glm::vec3 pos { 0, 0, 0 };
        alignas(4) glm::vec3 direction { 0, 0, 1 };
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 project;
    } data;
    float yaw, pitch;
    float speed = 2, sensi = 7;
    bool is_capture_cursor = false;
    std::shared_ptr<Match::UniformBuffer> uniform;
};
