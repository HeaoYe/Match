#include <Match/vulkan/descriptor_resource/spec_texture.hpp>
#include <Match/vulkan/resource/buffer.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/core/setting.hpp>
#include "../inner.hpp"
#include <stb/stb_image.h>

namespace Match {
    void Texture::load_error(const std::string &filename) {
        MCH_FATAL("Failed to load texture: {}", filename);
    }

    DataTexture::DataTexture(const uint8_t *data, uint32_t width, uint32_t height, uint32_t mip_levels) {
        uint32_t size = width * height * 4;
        Buffer buffer(size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
        memcpy(buffer.map(), data, size);
        buffer.unmap();
        if (mip_levels == 0) {
            mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))) + 1);
        }
        this->mip_levels = mip_levels;
        image = std::make_unique<Image>(width, height, vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst, vk::SampleCountFlagBits::e1, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, mip_levels);

        transition_image_layout(image->image, vk::ImageAspectFlagBits::eColor, mip_levels, { vk::ImageLayout::eUndefined, vk::AccessFlagBits::eNone, vk::PipelineStageFlagBits::eTopOfPipe }, { vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTransfer });

        auto command_buffer = manager->command_pool->allocate_single_use();
        vk::BufferImageCopy copy {};
        copy.setBufferOffset(0)
            .setBufferRowLength(0)
            .setBufferImageHeight(0)
            .setImageSubresource({
                vk::ImageAspectFlagBits::eColor,
                0,
                0,
                1
            })
            .setImageOffset({ 0, 0, 0 })
            .setImageExtent({ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 });
        command_buffer.copyBufferToImage(
            buffer.buffer,
            image->image,
            vk::ImageLayout::eTransferDstOptimal,
            { copy }
        );
        manager->command_pool->free_single_use(command_buffer);

        vk::FormatProperties format_properties;
        manager->device->physical_device.getFormatProperties(vk::Format::eR8G8B8A8Srgb, &format_properties);
        if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
            MCH_ERROR("Texture image format does not support linear blitting.");
        }

        command_buffer = manager->command_pool->allocate_single_use();
        vk::ImageMemoryBarrier barrier {};
        barrier.setImage(image->image)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setSubresourceRange({
                vk::ImageAspectFlagBits::eColor,
                0,
                1,
                0,
                1
            });
        int32_t mip_width = width, mip_height = height;
        for (uint32_t level = 1; level < mip_levels; level++) {
            barrier.subresourceRange.setBaseMipLevel(level - 1);
            barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
                .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setDstAccessMask(vk::AccessFlagBits::eTransferRead);
            command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion, {}, {}, { barrier });

            vk::ImageBlit blit {};
            blit.setSrcOffsets({ { { 0, 0, 0 }, { mip_width, mip_height, 1 } } })
                .setSrcSubresource({
                    vk::ImageAspectFlagBits::eColor,
                    level - 1,
                    0,
                    1
                })
                .setDstOffsets({ { { 0, 0, 0 } , { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 } } })
                .setDstSubresource({
                    vk::ImageAspectFlagBits::eColor,
                    level,
                    0,
                    1
                });
            command_buffer.blitImage(image->image, vk::ImageLayout::eTransferSrcOptimal, image->image, vk::ImageLayout::eTransferDstOptimal, { blit }, vk::Filter::eLinear);

            barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
                .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
                .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
            command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlagBits::eByRegion, {}, {}, { barrier });
            if (mip_width > 1) {
                mip_width /= 2;
            }
            if (mip_height > 1) {
                mip_height /= 2;
            }
        }
        barrier.subresourceRange.setBaseMipLevel(mip_levels - 1);
        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlagBits::eByRegion, {}, {}, { barrier });
        manager->command_pool->free_single_use(command_buffer);

        image_view = create_image_view(image->image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mip_levels);
    }

    DataTexture::~DataTexture() {
        manager->device->device.destroyImageView(image_view);
        image.reset();
    }

    ImgTexture::ImgTexture(const std::string &filename, uint32_t mip_levels) {
        int width, height, channels;
        stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels) {
            load_error(filename);
        }
        texture = std::make_unique<DataTexture>(pixels, width, height, mip_levels);
        stbi_image_free(pixels);
    }

    ImgTexture::~ImgTexture() {
        texture.reset();
    }

    KtxTexture::KtxTexture(const std::string &filename) {
        auto result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &texture);
        if (result != KTX_SUCCESS) {
            load_error(filename);
            return;
        }
        ktxVulkanDeviceInfo kvdi;
        ktxVulkanDeviceInfo_Construct(&kvdi, manager->device->physical_device, manager->device->device, manager->device->transfer_queue, manager->command_pool->command_pool, nullptr);
        result = ktxTexture_VkUploadEx(texture, &kvdi, &vk_texture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        if (result != KTX_SUCCESS) {
            load_error(filename);
            return;
        }
        image_view = create_image_view(vk_texture.image, vk::Format(vk_texture.imageFormat), vk::ImageAspectFlagBits::eColor, vk_texture.levelCount, vk_texture.layerCount, vk::ImageViewType(vk_texture.viewType));
        ktxVulkanDeviceInfo_Destruct(&kvdi);
    }

    KtxTexture::~KtxTexture() {
        vk_texture.vkFreeMemory(manager->device->device, vk_texture.deviceMemory, nullptr);
        vk_texture.vkDestroyImage(manager->device->device, vk_texture.image, nullptr);
        ktxTexture_Destroy(texture);
        manager->device->device.destroyImageView(image_view);
    }
}
