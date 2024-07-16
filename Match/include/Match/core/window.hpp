#pragma once

#include <Match/commons.hpp>
#include <GLFW/glfw3.h>

namespace Match {
    class Window {
        no_copy_move_construction(Window)
    public:
        MATCH_API Window();
        GLFWwindow *get_glfw_window() const { return window; }
        bool is_alive() const { return !glfwWindowShouldClose(window); }
        MATCH_API void poll_events() const;
        MATCH_API void close() const;
        MATCH_API ~Window();
    INNER_VISIBLE:
        GLFWwindow *window;
    };

    MATCH_API extern std::unique_ptr<Window> window;
}
