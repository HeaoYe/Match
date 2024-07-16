#pragma once
#include <Match/vulkan/descriptor_resource/texture.hpp>
#include <Match/vulkan/resource/image.hpp>
#include <ktxvulkan.h>

namespace Match {
    class DataTexture final : public Texture {
        no_copy_move_construction(DataTexture)
    public:
        MATCH_API DataTexture(const uint8_t *data, uint32_t width, uint32_t height, uint32_t mip_levels);
        vk::ImageLayout get_image_layout() override { return vk::ImageLayout::eShaderReadOnlyOptimal; }
        vk::ImageView get_image_view() override { return image_view; }
        uint32_t get_mip_levels() override { return 1; }
        MATCH_API ~DataTexture() override;
    INNER_VISIBLE:
        std::unique_ptr<Image> image;
        vk::ImageView image_view;
        uint32_t mip_levels;
    };

    class ImgTexture final : public Texture {
        no_copy_move_construction(ImgTexture)
    public:
        MATCH_API ImgTexture(const std::string &filename, uint32_t mip_levels);
        vk::ImageLayout get_image_layout() override { return vk::ImageLayout::eShaderReadOnlyOptimal; }
        vk::ImageView get_image_view() override { return texture->image_view; }
        uint32_t get_mip_levels() override { return texture->mip_levels; }
        MATCH_API ~ImgTexture() override;
    INNER_VISIBLE:
        std::unique_ptr<DataTexture> texture;
    };

    class KtxTexture final : public Texture {
        no_copy_move_construction(KtxTexture)
    public:
        MATCH_API KtxTexture(const std::string &filename);
        vk::ImageLayout get_image_layout() override { return vk::ImageLayout::eShaderReadOnlyOptimal; }
        vk::ImageView get_image_view() override { return image_view; }
        uint32_t get_mip_levels() override { return vk_texture.levelCount; }
        MATCH_API ~KtxTexture() override;
    INNER_VISIBLE:
        ktxTexture *texture;
        ktxVulkanTexture vk_texture;
        vk::ImageView image_view;
    };
}
