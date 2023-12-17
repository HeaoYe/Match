#pragma once

#include <Match/vulkan/commons.hpp>

#if defined (MATCH_WAYLAND)
#include <wayland-client.h>
#endif
#if defined (MATCH_XCB)
#include <xcb/xcb.h>
#endif
#if defined (MATCH_XLIB)
#include <X11/Xlib.h>
#endif
#if defined (MATCH_WIN32)
#include <Windows.h>
#endif

namespace Match {
    struct APIInfo {
    #if defined (MATCH_WAYLAND)
        wl_surface *wayland_surface;
        wl_display *wayland_display;
    #endif
    #if defined (MATCH_XCB)
        xcb_connection_t *xcb_connection;
        xcb_window_t xcb_window;
    #endif
    #if defined (MATCH_XLIB)
        Display *xlib_display;
        Window xlib_window;
    #endif
    #if defined (MATCH_WIN32)
        HINSTANCE win32_hinstance;
        HWND win32_hwnd;
    #endif
        VkSurfaceFormatKHR expect_format = { VK_FORMAT_MAX_ENUM, VK_COLOR_SPACE_MAX_ENUM_KHR };
        VkPresentModeKHR expect_present_mode = VK_PRESENT_MODE_MAX_ENUM_KHR;
    };
}
