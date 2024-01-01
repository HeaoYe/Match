#pragma once
#include <Match/vulkan/descriptor_resource/texture.hpp>
#include <Match/vulkan/resource/image.hpp>
#include <ktxvulkan.h>

namespace Match {
    class DataTexture final : public Texture {
        no_copy_move_construction(DataTexture)
    public:
        DataTexture(const uint8_t *data, uint32_t width, uint32_t height, uint32_t mip_levels);
        vk::Image get_image() override { return image->image; }
        vk::ImageView get_image_view() override { return image_view; }
        uint32_t get_mip_levels() override { return 1; }
        ~DataTexture() override;
    INNER_VISIBLE:
        std::unique_ptr<Image> image;
        vk::ImageView image_view;
        uint32_t mip_levels;
    };

    class ImgTexture final : public Texture {
        no_copy_move_construction(ImgTexture)
    public:
        ImgTexture(const std::string &filename, uint32_t mip_levels);
        vk::Image get_image() override { return texture->image->image; }
        vk::ImageView get_image_view() override { return texture->image_view; }
        uint32_t get_mip_levels() override { return texture->mip_levels; }
        ~ImgTexture() override;
    INNER_VISIBLE:
        std::unique_ptr<DataTexture> texture;
    };

    class KtxTexture final : public Texture {
        no_copy_move_construction(KtxTexture)
    public:
        KtxTexture(const std::string &filename);
        vk::Image get_image() override { return vk_texture.image; }
        vk::ImageView get_image_view() override { return image_view; }
        uint32_t get_mip_levels() override { return vk_texture.levelCount; }
        ~KtxTexture() override;
    INNER_VISIBLE:
        ktxTexture *texture;
        ktxVulkanTexture vk_texture;
        vk::ImageView image_view;
    };
}
