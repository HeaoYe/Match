#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class Image {
        no_copy_move_construction(Image)
    public:
        Image(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage, vk::SampleCountFlagBits samples, VmaMemoryUsage vma_usage, VmaAllocationCreateFlags vma_flags, uint32_t mip_levels = 1);
        ~Image();
    INNER_VISIBLE:
        vk::Image image;
        VmaAllocation allocation;
    };
}
