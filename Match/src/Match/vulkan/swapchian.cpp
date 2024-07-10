#include <Match/vulkan/swapchain.hpp>
#include <Match/core/setting.hpp>
#include <Match/vulkan/utils.hpp>
#include "inner.hpp"

namespace Match {
    Swapchain::Swapchain() {
        SwapchainDetails details;
        query_swapchain_details(details);

        uint32_t width = std::clamp<uint32_t>(details.capabilities.currentExtent.width, details.capabilities.minImageExtent.width, details.capabilities.maxImageExtent.width);
        uint32_t height = std::clamp<uint32_t>(details.capabilities.currentExtent.height, details.capabilities.minImageExtent.height, details.capabilities.maxImageExtent.height);
        runtime_setting->resize({ width, height });

        format = details.formats[0];
        present_mode = details.present_modes[0];

        vk::SurfaceFormatKHR expect_format;
        if (setting.expect_format.has_value()) {
            expect_format = setting.expect_format.value();
        } else {
            expect_format.format = vk::Format::eB8G8R8A8Unorm;
            expect_format.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
        }
        for (const auto &format : details.formats) {
            if (format.format == expect_format.format && format.colorSpace == expect_format.colorSpace) {
                MCH_DEBUG("Found expected surface format")
                this->format = format;
                break;
            }
        }

        vk::PresentModeKHR expect_present_mode;
        if (setting.expect_present_mode.has_value()) {
            expect_present_mode = setting.expect_present_mode.value();
        } else {
            if (runtime_setting->is_vsync()) {
                expect_present_mode = vk::PresentModeKHR::eFifo;
            } else {
                expect_present_mode = vk::PresentModeKHR::eMailbox;
            }
        }

        bool found = false;
        for (auto present_mode : details.present_modes) {
            if (present_mode == expect_present_mode) {
                MCH_DEBUG("Found expected present mode")
                this->present_mode = present_mode;
                found = true;
                break;
            }
        }
        if (!found) {
            for (auto present_mode : details.present_modes) {
                if (present_mode == vk::PresentModeKHR::eImmediate) {
                    MCH_DEBUG("Select immediate present mode")
                    this->present_mode = vk::PresentModeKHR::eImmediate;
                    break;
                }
            }
        }

        switch (present_mode) {
        case vk::PresentModeKHR::eMailbox:
            image_count = 4;
            break;
        case vk::PresentModeKHR::eFifo:
        case vk::PresentModeKHR::eImmediate:
        default:
            image_count = 3;
        }
        image_count = std::clamp(image_count, details.capabilities.minImageCount, details.capabilities.maxImageCount);
        MCH_DEBUG("Auto set swapchain image_count to {}", image_count);

        vk::SwapchainCreateInfoKHR swapchain_create_info {};
        swapchain_create_info.setOldSwapchain(VK_NULL_HANDLE)
            .setClipped(VK_TRUE)
            .setSurface(manager->surface)
            .setPreTransform(details.capabilities.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(present_mode)
            .setImageFormat(format.format)
            .setImageColorSpace(format.colorSpace)
            .setImageArrayLayers(1)
            .setImageExtent({ width, height })
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setMinImageCount(image_count);
        uint32_t queue_family_indices[] = { manager->device->graphics_family_index, manager->device->present_family_index };
        if (queue_family_indices[0] == queue_family_indices[1]) {
            swapchain_create_info.setImageSharingMode(vk::SharingMode::eExclusive);
        } else {
            swapchain_create_info.setImageSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndices(queue_family_indices);
        }
        swapchain = manager->device->device.createSwapchainKHR(swapchain_create_info);
        images = manager->device->device.getSwapchainImagesKHR(swapchain);
        image_views.resize(image_count);
        for (uint32_t i = 0; i < image_count; i ++) {
            image_views[i] = create_image_view(images[i], format.format, vk::ImageAspectFlagBits::eColor, 1);
        }

        switch (present_mode) {
        case vk::PresentModeKHR::eFifo:
            MCH_DEBUG("V-Sync ON (FIFO) real image_count is {}", image_count)
            break;
        case vk::PresentModeKHR::eImmediate:
            MCH_DEBUG("V-Sync OFF (IMMEDIATE) real image_count is {}", image_count)
            break;
        case vk::PresentModeKHR::eMailbox:
            MCH_DEBUG("V-Sync OFF (MailBox) real image_count is {}", image_count)
            break;
        default:
            MCH_DEBUG("Unknown present mode real image_count is {}", image_count)
        }
    }

    Swapchain::~Swapchain() {
        for (auto image_view : image_views) {
            manager->device->device.destroyImageView(image_view);
        }
        images.clear();
        image_views.clear();
        manager->device->device.destroySwapchainKHR(swapchain);
    }

    void Swapchain::query_swapchain_details(SwapchainDetails &details) {
        details.capabilities = manager->device->physical_device.getSurfaceCapabilitiesKHR(manager->surface);
        details.formats = manager->device->physical_device.getSurfaceFormatsKHR(manager->surface);
        details.present_modes = manager->device->physical_device.getSurfacePresentModesKHR(manager->surface);
    }
}
