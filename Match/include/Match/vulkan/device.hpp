#pragma once

#include <Match/commons.hpp>
#include <Match/vulkan/commons.hpp>

namespace Match {
    class Device {
        no_copy_move_construction(Device)
    public:
        Device();
        std::vector<std::string> enumerate_devices() const;
        VkFormat get_supported_depth_format() const;
        ~Device();
    private:
        bool check_device_suitable(VkPhysicalDevice device);
    INNER_VISIBLE:
        VkPhysicalDevice physical_device;
        VkDevice device;
        VkQueue graphics_queue;
        uint32_t graphics_family_index = -1;
        VkQueue present_queue;
        uint32_t present_family_index = -1;
        VkQueue compute_queue;
        uint32_t compute_family_index = -1;
        VkQueue transfer_queue;
        uint32_t transfer_family_index = -1;    
    };
}
