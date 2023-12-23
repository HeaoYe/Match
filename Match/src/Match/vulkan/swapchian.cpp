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

        // VkSurfaceFormatKHR expect_format = info.expect_format;
        // if (expect_format.format == VK_FORMAT_MAX_ENUM) {
        //     expect_format.format = VK_FORMAT_B8G8R8A8_UNORM;
        // }
        // if (expect_format.colorSpace == VK_COLOR_SPACE_MAX_ENUM_KHR) {
        //     expect_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        // }
        VkSurfaceFormatKHR expect_format { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        for (const auto &format : details.formats) {
            if (format.format == expect_format.format && format.colorSpace == expect_format.colorSpace) {
                MCH_DEBUG("Found expected surface format")
                this->format = format;
                break;
            }
        }

        // // VkPresentModeKHR expect_present_mode = info.expect_present_mode;
        // if (expect_present_mode == VK_PRESENT_MODE_MAX_ENUM_KHR) {
        //     if (runtime_setting->is_vsync()) {
        //         expect_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        //     } else {
        //         expect_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        //     }
        // }
        VkPresentModeKHR expect_present_mode;
        if (runtime_setting->vsync) {
            expect_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        } else {
            expect_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }

        for (auto present_mode : details.present_modes) {
            if (present_mode == expect_present_mode) {
                MCH_DEBUG("Found expected present mode")
                this->present_mode = present_mode;
                break;
            }
        }
        
        switch (present_mode) {
        case VK_PRESENT_MODE_MAILBOX_KHR:
            image_count = 4;
            break;
        case VK_PRESENT_MODE_FIFO_KHR:
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
        default:
            image_count = 3;
        }
        image_count = std::clamp(image_count, details.capabilities.minImageCount, details.capabilities.maxImageCount);
        MCH_DEBUG("Auto set swapchain image_count to {}", image_count);

        VkSwapchainCreateInfoKHR swapchain_create_info { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapchain_create_info.oldSwapchain = swapchain;
        swapchain_create_info.clipped = VK_TRUE;
        swapchain_create_info.surface = manager->surface;
        swapchain_create_info.preTransform = details.capabilities.currentTransform;
        swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_create_info.presentMode = present_mode;
        swapchain_create_info.imageFormat = format.format;
        swapchain_create_info.imageColorSpace = format.colorSpace;
        swapchain_create_info.imageArrayLayers = 1;
        swapchain_create_info.imageExtent = { width, height };
        swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_create_info.minImageCount = image_count;
        uint32_t queue_family_indices[] = { manager->device->graphics_family_index, manager->device->present_family_index };
        if (queue_family_indices[0] == queue_family_indices[1]) {
            swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        } else {
            swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
            swapchain_create_info.queueFamilyIndexCount = 2;
        }
        vk_assert(vkCreateSwapchainKHR(manager->device->device, &swapchain_create_info, manager->allocator, &swapchain))
        vkGetSwapchainImagesKHR(manager->device->device, swapchain, &image_count, nullptr);

        switch (present_mode) {
        case VK_PRESENT_MODE_MAILBOX_KHR:
            MCH_DEBUG("V-Sync ON (MailBox) real image_count is {}", image_count)
            break;
        case VK_PRESENT_MODE_FIFO_KHR:
            MCH_DEBUG("V-Sync ON (FIFO) real image_count is {}", image_count)
            break;
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            MCH_DEBUG("V-Sync OFF (IMMEDIATE) real image_count is {}", image_count)
            break;
        default:
            MCH_DEBUG("Unknown present mode real image_count is {}", image_count)
        }

        images.resize(image_count);
        image_views.resize(image_count);
        vkGetSwapchainImagesKHR(manager->device->device, swapchain, &image_count, images.data());
        for (uint32_t i = 0; i < image_count; i ++) {
            image_views[i] = create_image_view(images[i], format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    Swapchain::~Swapchain() {
        for (auto image_view : image_views) {
            vkDestroyImageView(manager->device->device, image_view, manager->allocator);
        }
        images.clear();
        image_views.clear();
        vkDestroySwapchainKHR(manager->device->device, swapchain, manager->allocator);
        swapchain = VK_NULL_HANDLE;
    }

    void Swapchain::query_swapchain_details(SwapchainDetails &details) {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(manager->device->physical_device, manager->surface, &details.capabilities);
        uint32_t format_count, present_mode_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(manager->device->physical_device, manager->surface, &format_count, nullptr);
        assert(format_count > 0);
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(manager->device->physical_device, manager->surface, &format_count, details.formats.data());
        vkGetPhysicalDeviceSurfacePresentModesKHR(manager->device->physical_device, manager->surface, &present_mode_count, nullptr);
        assert(present_mode_count > 0);
        details.present_modes.resize(format_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(manager->device->physical_device, manager->surface, &present_mode_count, details.present_modes.data());
    }
}
