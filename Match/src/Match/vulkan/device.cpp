#include <Match/vulkan/device.hpp>
#include <Match/core/setting.hpp>
#include <Match/vulkan/utils.hpp>
#include "inner.hpp"

namespace Match {
    std::vector<std::string> Device::enumerate_devices() const {
        uint32_t device_count;
        vkEnumeratePhysicalDevices(manager->instance, &device_count, nullptr);
        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(manager->instance, &device_count, devices.data());
        std::vector<std::string> device_names;
        VkPhysicalDeviceProperties properties;
        device_count = 0;
        for (auto &device : devices) {
            vkGetPhysicalDeviceProperties(device, &properties);
            MCH_WARN("No.{} \"{}\"", device_count, properties.deviceName)
            device_count ++;
        }
        return device_names;
    }
    
    VkFormat Device::get_supported_depth_format() const {
        return find_supported_format({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    Device::Device() {
        bool selected_device = false;
        if (setting.device_name != "auto") {
            uint32_t device_count;
            vkEnumeratePhysicalDevices(manager->instance, &device_count, nullptr);
            std::vector<VkPhysicalDevice> devices(device_count);
            vkEnumeratePhysicalDevices(manager->instance, &device_count, devices.data());
            VkPhysicalDeviceProperties properties;
            bool found_device = false;
            for (auto &device : devices) {
                vkGetPhysicalDeviceProperties(device, &properties);
                if (properties.deviceName == setting.device_name) {
                    MCH_DEBUG("Found device {}", properties.deviceName)
                    if (check_device_suitable(device)) {
                        selected_device = true;
                    }
                    found_device = true;
                    break;
                }
            }
            if (!found_device) {
                MCH_DEBUG("Cannot find device {}", setting.device_name)
            }
        }
        if ((setting.device_name == "auto") || (!selected_device)) {
            MCH_DEBUG("Auto find device")
            uint32_t device_count;
            selected_device = false;
            vkEnumeratePhysicalDevices(manager->instance, &device_count, nullptr);
            std::vector<VkPhysicalDevice> devices(device_count);
            vkEnumeratePhysicalDevices(manager->instance, &device_count, devices.data());
            VkPhysicalDeviceProperties properties;
            for (auto &device : devices) {
                if (check_device_suitable(device)) {
                    selected_device = true;
                    break;
                }
            }
            if (!selected_device) {
                MCH_FATAL("Cannot find suitable device on this pc")
            }
        }
        
        std::set<uint32_t> queue_family_indices = { graphics_family_index, present_family_index, transfer_family_index, compute_family_index };
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos(queue_family_indices.size());
        uint32_t i = 0;
        float priorities = 1.0f;
        for (auto queue_family_index : queue_family_indices) {
            queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_infos[i].pNext = nullptr;
            queue_create_infos[i].flags = 0;
            queue_create_infos[i].queueFamilyIndex = queue_family_index;
            queue_create_infos[i].queueCount = 1;
            queue_create_infos[i].pQueuePriorities = &priorities;
            i ++;
        }

        VkPhysicalDeviceFeatures features {};
        features.samplerAnisotropy = VK_TRUE;
        features.depthClamp = VK_TRUE;

        VkDeviceCreateInfo device_create_info { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        device_create_info.queueCreateInfoCount = queue_create_infos.size();
        device_create_info.pQueueCreateInfos = queue_create_infos.data();
        const char *extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        device_create_info.enabledExtensionCount = 1;
        device_create_info.ppEnabledExtensionNames = extensions;
        const char *layers[] = { "VK_LAYER_KHRONOS_validation" };
        if (setting.debug_mode) {
            device_create_info.enabledLayerCount = 1;
            device_create_info.ppEnabledLayerNames = layers;
        } else {
            device_create_info.enabledLayerCount = 0;
            device_create_info.ppEnabledLayerNames = nullptr;
        }
        device_create_info.pEnabledFeatures = &features;
        vk_assert(vkCreateDevice(physical_device, &device_create_info, manager->allocator, &device))

        vkGetDeviceQueue(device, graphics_family_index, 0, &graphics_queue);
        vkGetDeviceQueue(device, present_family_index, 0, &present_queue);
        vkGetDeviceQueue(device, transfer_family_index, 0, &transfer_queue);
        vkGetDeviceQueue(device, compute_family_index, 0, &compute_queue);
    }

    Device::~Device() {
        vkDestroyDevice(device, manager->allocator);
    }

    bool Device::check_device_suitable(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);

        if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            MCH_DEBUG("{} is not a discrete gpu -- skipping", properties.deviceName)
            return false;
        }

        if (!features.samplerAnisotropy) {
            MCH_DEBUG("{} does not support sampler anisotropy -- skipping", properties.deviceName)
            return false;
        }

        uint32_t queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families_properties(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families_properties.data());
        queue_family_count = 0;
        bool found_family = false;
        for (const auto &queue_family_properties : queue_families_properties) {
            if (queue_family_properties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT)) {
                VkBool32 supported;
                vk_check(vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_family_count, manager->surface, &supported))
                if (supported) {
                    graphics_family_index = queue_family_count;
                    present_family_index = queue_family_count;
                    compute_family_index = queue_family_count;
                    transfer_family_index = queue_family_count;
                    found_family = true;
                    break;
                }
            }
            queue_family_count++;
        }

        if (!found_family) {
            MCH_DEBUG("{} has no suitable queue family -- skipping", properties.deviceName)
            return false;
        }

        MCH_DEBUG("{} is suitable", properties.deviceName)
        physical_device = device;
        MCH_INFO("Select Device {}", properties.deviceName)

        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memoryProperties);
        MCH_DEBUG("\tDevice Driver Version: {}.{}.{}", VK_VERSION_MAJOR(properties.driverVersion), VK_VERSION_MINOR(properties.driverVersion), VK_VERSION_PATCH(properties.driverVersion))
        MCH_DEBUG("\tVulkan API Version: {}.{}.{}", VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion))
        for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++) {
            float size = ((float)memoryProperties.memoryHeaps[i].size) / 1024.0f / 1024.0f / 1024.0f;
            if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                MCH_DEBUG("\tLocal GPU Memory: {} Gib", size)
            }
            else {
                MCH_DEBUG("\tShared GPU Memory: {} Gib", size)
            }
        }
        return true;
    }

}
