#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class Image {
        no_copy_move_construction(Image)
    public:
        Image(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples, VmaMemoryUsage vma_usage, VmaAllocationCreateFlags vma_flags);
        ~Image();
    INNER_VISIBLE:
        VkImage image;
        VmaAllocation allocation;
    };
}
