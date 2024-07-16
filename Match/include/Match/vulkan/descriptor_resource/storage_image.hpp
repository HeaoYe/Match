#pragma once

#include <Match/vulkan/descriptor_resource/texture.hpp>
#include <Match/vulkan/resource/image.hpp>

namespace Match {
    class StorageImage : public Texture {
        no_copy_move_construction(StorageImage)
    public:
        MATCH_API StorageImage(uint32_t width, uint32_t height, vk::Format format, bool sampled, bool enable_clear);
        vk::ImageLayout get_image_layout() override { return vk::ImageLayout::eGeneral; }
        vk::ImageView get_image_view() override { return image_view; };
        uint32_t get_mip_levels() override { return 1; };
        MATCH_API ~StorageImage();
    INNER_VISIBLE:
        std::unique_ptr<Image> image;
        vk::ImageView image_view;
    };
}
