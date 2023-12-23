#pragma once

#include <Match/commons.hpp>
#include <GLFW/glfw3.h>

namespace Match {
    class Window {
        no_copy_move_construction(Window)
    public:
        Window();
        GLFWwindow *get_glfw_window() const { return window; }
        ~Window();
    INNER_VISIBLE:
        GLFWwindow *window;
    };

    extern std::unique_ptr<Window> window;
}
