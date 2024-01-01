#include <Match/vulkan/resource/image.hpp>
#include "../inner.hpp"

namespace Match {
    Image::Image(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage, vk::SampleCountFlagBits samples, VmaMemoryUsage vma_usage, VmaAllocationCreateFlags vma_flags, uint32_t mip_levels) {
        vk::ImageCreateInfo image_create_info {};
        image_create_info.setImageType(vk::ImageType::e2D)
            .setExtent({
                width,
                height,
                1
            })
            .setMipLevels(mip_levels)
            .setArrayLayers(1)
            .setFormat(format)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(usage)
            .setSamples(samples)
            .setInitialLayout(vk::ImageLayout::eUndefined);
        VmaAllocationCreateInfo alloc_info {};
        alloc_info.usage = vma_usage;
        alloc_info.flags = vma_flags;
        auto res = vmaCreateImage(manager->vma_allocator, reinterpret_cast<VkImageCreateInfo *>(&image_create_info), &alloc_info, reinterpret_cast<VkImage *>(&image), &allocation, nullptr);

    }

    Image::~Image() {
        vmaDestroyImage(manager->vma_allocator, image, allocation);
    }
}
