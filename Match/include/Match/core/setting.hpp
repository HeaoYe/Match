#pragma once

#include <Match/vulkan/commons.hpp>
#include <optional>

namespace Match {
    struct Setting {
        bool debug_mode = false;
        std::string device_name = "auto";
        std::string app_name = "Match App";
        uint32_t app_version = 1;
        std::string engine_name = "No Engine";
        uint32_t engine_version = 1;
        std::optional<vk::SurfaceFormatKHR> expect_format;
        std::optional<vk::PresentModeKHR> expect_present_mode;
        std::array<uint32_t, 2> window_pos = { 100, 50 };
        std::array<uint32_t, 2> window_size = { 800, 800 };
        uint32_t max_in_flight_frame = 2;
        std::string default_font_filename = "";
        std::string chinese_font_filename = "";
        float font_size = 13.0f;
    };

    class RuntimeSetting {
        default_no_copy_move_construction(RuntimeSetting)
    public:
        RuntimeSetting &set_vsync(bool n);
        bool is_vsync() { return vsync; }
        const WindowSize &get_window_size() { return window_size; }
        bool is_msaa_enabled();
        RuntimeSetting &set_multisample_count(SampleCount count);
        SampleCount get_multisample_count() { return multisample_count; }
    INNER_VISIBLE:
        void resize(const WindowSize &size);
    INNER_VISIBLE:
        bool vsync = true;
        WindowSize window_size;
        SampleCount multisample_count = SampleCount::e1;
        uint32_t current_in_flight = 0;
    };

    extern Setting setting;
    extern std::shared_ptr<RuntimeSetting> runtime_setting;
}
