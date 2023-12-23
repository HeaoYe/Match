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
        vk_check(vkCreateImageView(manager->device->device, &create_info, manager->allocator, &view))
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

    void transition_image_layout(VkImage image, VkImageAspectFlags aspect, uint32_t mip_levels, const TransitionInfo &src, const TransitionInfo &dst) {
        auto command_buffer = manager->command_pool->allocate_single_use();
        VkImageMemoryBarrier barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout = src.layout;
        barrier.srcAccessMask = src.access;
        barrier.newLayout = dst.layout;
        barrier.dstAccessMask = dst.access;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = aspect;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mip_levels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkCmdPipelineBarrier(
            command_buffer,
            src.stage,
            dst.stage,
            0, 0, nullptr, 0, nullptr,
            1, &barrier
        );
        manager->command_pool->free_single_use(command_buffer);
    }

    VkFormat get_supported_depth_format() {
        return find_supported_format({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    bool has_stencil_component(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    VkSampleCountFlagBits get_max_usable_sample_count() {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(manager->device->physical_device, &properties);

        VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }
}
