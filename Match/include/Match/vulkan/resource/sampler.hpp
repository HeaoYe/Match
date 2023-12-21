#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    enum class SamplerFilter {
        eNearest,
        eLinear,
    };

    enum class SamplerAddressMode {
        eRepeat,
        eMirroredRepeat,
        eClampToEdge,
        eClampToBorder,
    };

    enum class SamplerBorderColor {
        eFloatTransparentBlack,
        eIntTransparentBlack,
        eFloatOpaqueBlack,
        eIntOpaqueBlack,
        eFloatOpaqueWhite,
        eIntOpaqueWhite,
    };

    struct SamplerOptions {
        SamplerFilter mag_filter = SamplerFilter::eLinear;
        SamplerFilter min_filter = SamplerFilter::eLinear;
        SamplerAddressMode address_mode_u = SamplerAddressMode::eRepeat;
        SamplerAddressMode address_mode_v = SamplerAddressMode::eRepeat;
        SamplerAddressMode address_mode_w = SamplerAddressMode::eRepeat;
        uint32_t max_anisotropy = 1;
        SamplerBorderColor border_color = SamplerBorderColor::eIntOpaqueBlack;
        SamplerFilter mipmap_mode = SamplerFilter::eLinear;
        uint32_t mipmap_levels = 1;
    };
    
    class Sampler {
        no_copy_move_construction(Sampler)
    public:
        Sampler(const SamplerOptions &options);
        ~Sampler();
    INNER_VISIBLE:
        VkSampler sampler;
    };
}
