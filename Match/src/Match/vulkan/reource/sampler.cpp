#include <Match/vulkan/resource/sampler.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"

namespace Match {
    Sampler::Sampler(const SamplerOptions &options) {
        VkSamplerCreateInfo sampler_create_info { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        sampler_create_info.magFilter = transform<VkFilter>(options.mag_filter);
        sampler_create_info.minFilter = transform<VkFilter>(options.min_filter);
        sampler_create_info.addressModeU = transform<VkSamplerAddressMode>(options.address_mode_u);
        sampler_create_info.addressModeV = transform<VkSamplerAddressMode>(options.address_mode_v);
        sampler_create_info.addressModeW = transform<VkSamplerAddressMode>(options.address_mode_w);
        sampler_create_info.anisotropyEnable = VK_TRUE;
        sampler_create_info.maxAnisotropy = options.max_anisotropy;
        sampler_create_info.borderColor = transform<VkBorderColor>(options.border_color);
        sampler_create_info.unnormalizedCoordinates = VK_FALSE;
        sampler_create_info.compareEnable = VK_FALSE;
        sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_create_info.mipmapMode = transform<VkSamplerMipmapMode>(options.mipmap_mode);
        sampler_create_info.minLod = 0.0f;
        sampler_create_info.maxLod = static_cast<float>(options.mipmap_levels);
        sampler_create_info.mipLodBias = 0.0f;
        vkCreateSampler(manager->device->device, &sampler_create_info, manager->allocator, &sampler);
    }

    Sampler::~Sampler() {
        vkDestroySampler(manager->device->device, sampler, manager->allocator);
    }
}
