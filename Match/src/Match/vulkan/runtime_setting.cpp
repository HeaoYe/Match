#include <Match/core/setting.hpp>

namespace Match {
    void RuntimeSetting::set_vsync(bool n) {
        MCH_FATAL("Set V-SYNC")
    }

    void RuntimeSetting::resize(const WindowSize &size) {
        window_size = size;
    }

    void RuntimeSetting::set_multisample_count(VkSampleCountFlagBits count) {
        this->multisample_count = count;
    }
}
