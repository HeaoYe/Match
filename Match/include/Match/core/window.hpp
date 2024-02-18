#pragma once

#include <Match/commons.hpp>
#include <GLFW/glfw3.h>

namespace Match {
    class MATCH_API Window {
        no_copy_move_construction(Window)
    public:
        Window();
        GLFWwindow *get_glfw_window() const { return window; }
        bool is_alive() const { return !glfwWindowShouldClose(window); }
        void poll_events() const;
        void close() const;
        ~Window();
    INNER_VISIBLE:
        GLFWwindow *window;
    };

    MATCH_API extern std::unique_ptr<Window> window;
}
