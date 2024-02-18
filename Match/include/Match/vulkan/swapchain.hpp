#pragma once
#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    struct MATCH_API SwapchainDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> present_modes;
    };

    class MATCH_API Swapchain {
        no_copy_move_construction(Swapchain)
    public:
        Swapchain();
        ~Swapchain();
    private:
        void query_swapchain_details(SwapchainDetails &details);
    INNER_VISIBLE:
        vk::SwapchainKHR swapchain;
        vk::SurfaceFormatKHR format;
        vk::PresentModeKHR present_mode;
        uint32_t image_count;
        std::vector<vk::Image> images;
        std::vector<vk::ImageView> image_views;
    };
}
