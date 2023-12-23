#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mipmap_levels);

    VkFormat find_supported_format(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    struct TransitionInfo {
        VkImageLayout layout;
        VkAccessFlags access;
        VkPipelineStageFlags stage;
    };

    void transition_image_layout(VkImage image, VkImageAspectFlags aspect, uint32_t mip_levels, const TransitionInfo &src, const TransitionInfo &dst);

    VkFormat get_supported_depth_format();

    bool has_stencil_component(VkFormat format);

    VkSampleCountFlagBits get_max_usable_sample_count();
}
