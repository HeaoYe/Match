#pragma once

#include <Match/commons.hpp>
#include <Match/vulkan/commons.hpp>

namespace Match {
    class MATCH_API Device {
        no_copy_move_construction(Device)
    public:
        Device();
        std::vector<std::string> enumerate_devices() const;
        ~Device();
    private:
        bool check_device_suitable(vk::PhysicalDevice device);
    INNER_VISIBLE:
        vk::PhysicalDevice physical_device;
        vk::Device device;
        vk::Queue graphics_queue;
        uint32_t graphics_family_index = -1;
        vk::Queue present_queue;
        uint32_t present_family_index = -1;
        vk::Queue compute_queue;
        uint32_t compute_family_index = -1;
        vk::Queue transfer_queue;
        uint32_t transfer_family_index = -1;    
    };
}
