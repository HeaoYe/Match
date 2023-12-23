#include <Match/core/window.hpp>
#include <Match/core/setting.hpp>

namespace Match {
    std::unique_ptr<Window> window;

    Window::Window() {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(setting.window_size[0], setting.window_size[1], setting.app_name.c_str(), nullptr, nullptr);
        glfwSetWindowPos(window, setting.window_pos[0], setting.window_pos[1]);
    }

    Window::~Window() {
        glfwDestroyWindow(window);
    }
}
