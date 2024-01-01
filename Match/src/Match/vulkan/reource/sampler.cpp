#include <Match/vulkan/resource/sampler.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"

namespace Match {
    Sampler::Sampler(const SamplerOptions &options) {
        vk::SamplerCreateInfo sampler_create_info {};
        sampler_create_info.setMagFilter(transform<vk::Filter>(options.mag_filter))
            .setMinFilter(transform<vk::Filter>(options.min_filter))
            .setAddressModeU(transform<vk::SamplerAddressMode>(options.address_mode_u))
            .setAddressModeV(transform<vk::SamplerAddressMode>(options.address_mode_v))
            .setAddressModeW(transform<vk::SamplerAddressMode>(options.address_mode_w))
            .setAnisotropyEnable(VK_TRUE)
            .setMaxAnisotropy(options.max_anisotropy)
            .setBorderColor(transform<vk::BorderColor>(options.border_color))
            .setUnnormalizedCoordinates(VK_FALSE)
            .setCompareOp(vk::CompareOp::eAlways)
            .setMipmapMode(transform<vk::SamplerMipmapMode>(options.mipmap_mode))
            .setMinLod(0.0f)
            .setMaxLod(static_cast<float>(options.mip_levels))
            .setMipLodBias(0.0f);
        sampler = manager->device->device.createSampler(sampler_create_info);
    }

    Sampler::~Sampler() {
        manager->device->device.destroySampler(sampler);
    }
}
