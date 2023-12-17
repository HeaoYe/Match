#pragma once
#pragma once

#include <Match/vulkan/commons.hpp>
#include <Match/vulkan/api_info.hpp>

namespace Match {
    struct SwapchainDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    class Swapchain {
        no_copy_move_construction(Swapchain)
    public:
        Swapchain(APIInfo &info);
        ~Swapchain();
    private:
        void query_swapchain_details(SwapchainDetails &details);
    INNER_VISIBLE:
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkSurfaceFormatKHR format;
        VkPresentModeKHR present_mode;
        uint32_t image_count;
        std::vector<VkImage> images;
        std::vector<VkImageView> image_views;
    };
}
