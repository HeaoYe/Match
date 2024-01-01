#include <Match/core/setting.hpp>
#include <Match/vulkan/utils.hpp>

namespace Match {
    RuntimeSetting &RuntimeSetting::set_vsync(bool n) {
        MCH_INFO("Set V-SYNC {}", n)
        vsync = n;
        return *this;
    }

    void RuntimeSetting::resize(const WindowSize &size) {
        window_size = size;
    }
    
    bool RuntimeSetting::is_msaa_enabled() {
        return multisample_count != SampleCount::e1;
    }

    RuntimeSetting &RuntimeSetting::set_multisample_count(SampleCount count) {
        static auto max_multisample_count = get_max_usable_sample_count();
        if (static_cast<int>(count) > static_cast<int>(max_multisample_count)) {
            MCH_WARN("multisample count {} > max multisample count {}", static_cast<uint32_t>(count), static_cast<uint32_t>(max_multisample_count))
            count = max_multisample_count;
        }
        multisample_count = count;
        return *this;
    }
}
