#include <Match/core/setting.hpp>
#include <Match/vulkan/utils.hpp>

namespace Match {
    void RuntimeSetting::set_vsync(bool n) {
        MCH_INFO("Set V-SYNC {}", n)
    }

    void RuntimeSetting::resize(const WindowSize &size) {
        window_size = size;
    }
    
    bool RuntimeSetting::is_msaa_enabled() {
        return multisample_count != VK_SAMPLE_COUNT_1_BIT;
    }

    void RuntimeSetting::set_multisample_count(VkSampleCountFlagBits count) {
        static auto max_multisample_count = get_max_usable_sample_count();
        if (count > max_multisample_count) {
            MCH_WARN("multisample count {} > max multisample count {}", static_cast<uint32_t>(count), static_cast<uint32_t>(max_multisample_count))
            count = max_multisample_count;
        }
        multisample_count = count;
    }
}
