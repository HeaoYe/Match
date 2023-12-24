#pragma once
#include <Match/vulkan/descriptor_resource/texture.hpp>
#include <Match/vulkan/resource/image.hpp>
#include <ktxvulkan.h>

namespace Match {
    class ImgTexture final : public Texture {
        no_copy_move_construction(ImgTexture)
    public:
        ImgTexture(const std::string &filename, uint32_t mip_levels);
        VkImage get_image() override { return image->image; }
        VkImageView get_image_view() override { return image_view; }
        uint32_t get_mip_levels() override { return mip_levels; }
        ~ImgTexture() override;
    INNER_VISIBLE:
        std::unique_ptr<Image> image;
        VkImageView image_view;
        uint32_t mip_levels;
    };

    class KtxTexture final : public Texture {
        no_copy_move_construction(KtxTexture)
    public:
        KtxTexture(const std::string &filename);
        VkImage get_image() override { return vk_texture.image; }
        VkImageView get_image_view() override { return image_view; }
        uint32_t get_mip_levels() override { return vk_texture.levelCount; }
        ~KtxTexture() override;
    INNER_VISIBLE:
        ktxTexture *texture;
        ktxVulkanTexture vk_texture;
        VkImageView image_view;
    };
}
