#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    struct Setting {
        bool debug_mode = false;
        std::string device_name = "auto";
        std::string app_name = "Match App";
        uint32_t app_version = 1;
        std::string engine_name = "No Engine";
        uint32_t engine_version = 1;
        PlatformWindowSystem render_backend = PlatformWindowSystem::eNone;
    };

    class RuntimeSetting {
        default_no_copy_move_construction(RuntimeSetting)
    public:
        void set_vsync(bool n);
        bool is_vsync() { return vsync; }
        void resize(const WindowSize &size);
        const WindowSize &get_window_size() { return window_size; }
        void set_multisample_count(VkSampleCountFlagBits count);
        VkSampleCountFlagBits get_multisample_count() { return VK_SAMPLE_COUNT_1_BIT; }
    private:
        bool vsync = true;
        WindowSize window_size;
        VkSampleCountFlagBits multisample_count = VK_SAMPLE_COUNT_1_BIT;
    };

    extern Setting setting;
    extern std::shared_ptr<RuntimeSetting> runtime_setting;
}
