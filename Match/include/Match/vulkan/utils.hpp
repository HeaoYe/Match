#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mipmap_levels);
    VkFormat find_supported_format(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
}
