#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    struct SamplerOptions {
        SamplerFilter mag_filter = SamplerFilter::eLinear;
        SamplerFilter min_filter = SamplerFilter::eLinear;
        SamplerAddressMode address_mode_u = SamplerAddressMode::eRepeat;
        SamplerAddressMode address_mode_v = SamplerAddressMode::eRepeat;
        SamplerAddressMode address_mode_w = SamplerAddressMode::eRepeat;
        uint32_t max_anisotropy = 1;
        SamplerBorderColor border_color = SamplerBorderColor::eIntOpaqueBlack;
        SamplerFilter mipmap_mode = SamplerFilter::eLinear;
        uint32_t mip_levels = 1;
    };
    
    class Sampler {
        no_copy_move_construction(Sampler)
    public:
        Sampler(const SamplerOptions &options);
        ~Sampler();
    INNER_VISIBLE:
        vk::Sampler sampler;
    };
}
