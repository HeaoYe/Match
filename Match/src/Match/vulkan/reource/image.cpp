#include <Match/vulkan/resource/image.hpp>
#include "../inner.hpp"

namespace Match {
    Image::Image(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkSampleCountFlagBits samples, VmaMemoryUsage vma_usage, VmaAllocationCreateFlags vma_flags, uint32_t mip_levels) {
        VkImageCreateInfo image_create_info { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.extent = { width, height, 1 };
        image_create_info.mipLevels = mip_levels;
        image_create_info.arrayLayers = 1;
        image_create_info.format = format;
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = usage;
        image_create_info.samples = samples;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VmaAllocationCreateInfo alloc_info {};
        alloc_info.usage = vma_usage;
        alloc_info.flags = vma_flags;
        auto res = vmaCreateImage(manager->vma_allocator, &image_create_info, &alloc_info, &image, &allocation, nullptr);

    }

    Image::~Image() {
        vmaDestroyImage(manager->vma_allocator, image, allocation);
    }
}
