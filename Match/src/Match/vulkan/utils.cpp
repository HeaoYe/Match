#include <Match/vulkan/utils.hpp>
#include "inner.hpp"

namespace Match {
    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mipmap_levels) {
        VkImageViewCreateInfo create_info { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        create_info.image = image;
        create_info.format = format;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.components = VkComponentMapping {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A,
        };
        create_info.subresourceRange = VkImageSubresourceRange {
            .aspectMask = aspect,
            .baseMipLevel = 0,
            .levelCount = mipmap_levels,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        VkImageView view;
        vk_check(vkCreateImageView(manager->device->device, &create_info, manager->alloctor, &view))
        return view;
    }
    
    VkFormat find_supported_format(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (const auto &format : candidates) {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(manager->device->physical_device, format, &properties);
            if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        MCH_ERROR("Failed to find supported vulkan format")
        return VK_FORMAT_UNDEFINED;
    }
}
