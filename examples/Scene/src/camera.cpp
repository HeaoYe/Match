#include "camera.hpp"
#include <glm/gtx/rotate_vector.hpp>

Camera::Camera(Match::ResourceFactory &factory) {
    uniform = factory.create_uniform_buffer(sizeof(CameraUniform));
    yaw = 0;
    pitch = 0;
    data.project = glm::perspective(glm::radians(60.0f), (float) Match::setting.window_size[0] / (float) Match::setting.window_size[1], 0.1f, 100.0f);
    upload_data();
}

void Camera::upload_data() {
    auto direction = glm::rotateY(glm::rotateX(glm::vec3(0, 0, 1), glm::radians(pitch)), glm::radians(yaw));
    data.view = glm::lookAt(data.pos, data.pos + direction, glm::vec3(.0f, 1.0f, .0f));
    memcpy(uniform->get_uniform_ptr(), &data, sizeof(CameraUniform));
}

void Camera::update(float dt) {
    static double last_x, last_y;
    static bool first = true;
    double xpos, ypos;
    glfwGetCursorPos(Match::window->get_glfw_window(), &xpos, &ypos);
    if (first) {
        first = false;
        last_x = xpos;
        last_y = ypos;
        return;
    }
    static bool capslock_down = false;
    auto capslock = glfwGetKey(Match::window->get_glfw_window(), GLFW_KEY_CAPS_LOCK);
    if (!capslock_down && capslock == GLFW_PRESS) {
        capslock_down = true;
    } else if (capslock_down && capslock == GLFW_RELEASE) {
        capslock_down = false;
        is_capture_cursor = !is_capture_cursor;
        glfwSetInputMode(Match::window->get_glfw_window(), GLFW_CURSOR, is_capture_cursor ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
    if (!is_capture_cursor) {
        last_x = xpos;
        last_y = ypos;
        return;
    }

    double delta_x = (xpos - last_x) * dt * sensi;
    double delta_y = (ypos - last_y) * dt * sensi;

    yaw -= delta_x;
    pitch = std::clamp<float>(pitch + delta_y, -89.0f, 89.0f);
    last_x = xpos;
    last_y = ypos;

    auto d = glm::rotateY(glm::vec3(0.0f, 0.0f, 1.0f), glm::radians(yaw));
    auto cd = glm::rotateY(d, glm::radians(90.0f));
    float move = speed * dt;
    if (glfwGetKey(Match::window->get_glfw_window(), GLFW_KEY_W))
        data.pos += d * move;
    if (glfwGetKey(Match::window->get_glfw_window(), GLFW_KEY_S))
        data.pos -= d * move;
    if (glfwGetKey(Match::window->get_glfw_window(), GLFW_KEY_A))
        data.pos += cd * move;
    if (glfwGetKey(Match::window->get_glfw_window(), GLFW_KEY_D))
        data.pos -= cd * move;
    if (glfwGetKey(Match::window->get_glfw_window(), GLFW_KEY_Q))
        data.pos.y += move;
    if (glfwGetKey(Match::window->get_glfw_window(), GLFW_KEY_E))
        data.pos.y -= move;

    upload_data();
}
