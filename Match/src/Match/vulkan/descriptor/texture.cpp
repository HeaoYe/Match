#include <Match/vulkan/descriptor/texture.hpp>
#include <Match/vulkan/resource/buffer.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/core/setting.hpp>
#include "../inner.hpp"
#include <stb/stb_image.h>

namespace Match {
    Texture::Texture(const std::string &filename) {
        int width, height, channels;
        stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels) {
            MCH_FATAL("Failed to load texture: {}", filename);
            return ;
        }
        uint32_t size = width * height * 4;
        Buffer buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
        memcpy(buffer.map(), pixels, size);
        buffer.unmap();
        stbi_image_free(pixels);
        image = std::make_unique<Image>(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, runtime_setting->get_multisample_count(), VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

        transition_image_layout(image->image, VK_IMAGE_ASPECT_COLOR_BIT, { VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT }, { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT });

        auto command_buffer = manager->command_pool->allocate_single_use();
        VkBufferImageCopy copy;
        copy.bufferOffset = 0;
        copy.bufferRowLength = 0;
        copy.bufferImageHeight = 0;

        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.mipLevel = 0;
        copy.imageSubresource.baseArrayLayer = 0;
        copy.imageSubresource.layerCount = 1;

        copy.imageOffset = { 0, 0, 0 };
        copy.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
        vkCmdCopyBufferToImage(
            command_buffer,
            buffer.buffer,
            image->image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copy
        );
        manager->command_pool->free_single_use(command_buffer);

        transition_image_layout(image->image, VK_IMAGE_ASPECT_COLOR_BIT, { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }, { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT });

        image_view = create_image_view(image->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    Texture::~Texture() {
        vkDestroyImageView(manager->device->device, image_view, manager->allocator);
        image.reset();
    }
}
