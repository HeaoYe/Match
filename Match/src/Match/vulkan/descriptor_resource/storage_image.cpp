#include <Match/vulkan/descriptor_resource/storage_image.hpp>
#include <Match/vulkan/utils.hpp>
#include "../inner.hpp"

namespace Match {
    StorageImage::StorageImage(uint32_t width, uint32_t height, vk::Format format, bool sampled, bool enable_clear) {
        vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage;
        if (sampled) {
            usage |= vk::ImageUsageFlagBits::eSampled;
        }
        if (enable_clear) {
            usage |= vk::ImageUsageFlagBits::eTransferDst;
        }
        image = std::make_unique<Image>(width, height, format, usage, vk::SampleCountFlagBits::e1, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, 1);
        transition_image_layout(image->image, vk::ImageAspectFlagBits::eColor, 1, { vk::ImageLayout::eUndefined, vk::AccessFlagBits::eNone, vk::PipelineStageFlagBits::eTopOfPipe }, { vk::ImageLayout::eGeneral, vk::AccessFlagBits::eMemoryRead, vk::PipelineStageFlagBits::eAllGraphics });
        image_view = create_image_view(image->image, format, vk::ImageAspectFlagBits::eColor, 1);
    }

    StorageImage::~StorageImage() {
        manager->device->device.destroyImageView(image_view);
        image.reset();
    }
}
