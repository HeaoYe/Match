#include <Match/vulkan/device.hpp>
#include <Match/core/setting.hpp>
#include <Match/vulkan/utils.hpp>
#include "inner.hpp"

namespace Match {
    std::vector<std::string> Device::enumerate_devices() const {
        auto devices = manager->instance.enumeratePhysicalDevices();
        std::vector<std::string> device_names;
        vk::PhysicalDeviceProperties properties;
        auto idx = 0;
        for (auto &device : devices) {
            properties = device.getProperties();
            MCH_WARN("No.{} \"{}\"", idx, properties.deviceName)
            idx ++;
        }
        return device_names;
    }

    Device::Device() {
        bool selected_device = false;
        if (setting.device_name != "auto") {
            auto devices = manager->instance.enumeratePhysicalDevices();
            vk::PhysicalDeviceProperties properties;
            bool found_device = false;
            for (auto &device : devices) {
                properties = device.getProperties();
                if (properties.deviceName.data() == setting.device_name) {
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
            selected_device = false;
            auto devices = manager->instance.enumeratePhysicalDevices();
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
        std::vector<vk::DeviceQueueCreateInfo> queue_create_infos(queue_family_indices.size());
        uint32_t i = 0;
        float priorities = 1.0f;
        for (auto queue_family_index : queue_family_indices) {
            queue_create_infos[i].setQueueFamilyIndex(queue_family_index)
                .setQueueCount(1)
                .setQueuePriorities(priorities);
            i ++;
        }

        vk::PhysicalDeviceFeatures features {};
        features.samplerAnisotropy = VK_TRUE;
        features.depthClamp = VK_TRUE;
        features.sampleRateShading = VK_TRUE;

        vk::DeviceCreateInfo device_create_info {};
        const char *extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        const char *layers[] = { "VK_LAYER_KHRONOS_validation" };
        device_create_info.setQueueCreateInfos(queue_create_infos)
            .setPEnabledExtensionNames(extensions)
            .setPEnabledFeatures(&features);
        if (setting.debug_mode) {
            device_create_info.setPEnabledLayerNames(layers);
        }
        device = physical_device.createDevice(device_create_info);

        graphics_queue = device.getQueue(graphics_family_index, 0);
        present_queue = device.getQueue(present_family_index, 0);
        transfer_queue = device.getQueue(transfer_family_index, 0);
        compute_queue = device.getQueue(compute_family_index, 0);
    }

    Device::~Device() {
        device.destroy();
    }

    bool Device::check_device_suitable(vk::PhysicalDevice device) {
        auto properties = device.getProperties();
        auto features = device.getFeatures();

        if (properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
            MCH_DEBUG("{} is not a discrete gpu -- skipping", properties.deviceName)
            return false;
        }

        if (!features.samplerAnisotropy) {
            MCH_DEBUG("{} does not support sampler anisotropy -- skipping", properties.deviceName)
            return false;
        }

        if (!features.sampleRateShading) {
            MCH_DEBUG("{} does not support sampler rate shading -- skipping", properties.deviceName)
            return false;
        }

        auto queue_families_properties = device.getQueueFamilyProperties();
        auto queue_family_idx = 0;
        bool found_family = false;
        for (const auto &queue_family_properties : queue_families_properties) {
            if ((queue_family_properties.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer | vk::QueueFlagBits::eCompute)) && 
                (device.getSurfaceSupportKHR(queue_family_idx, manager->surface))) {
                graphics_family_index = queue_family_idx;
                present_family_index = queue_family_idx;
                compute_family_index = queue_family_idx;
                transfer_family_index = queue_family_idx;
                found_family = true;
                break;
            }
            queue_family_idx++;
        }

        if (!found_family) {
            MCH_DEBUG("{} has no suitable queue family -- skipping", properties.deviceName)
            return false;
        }

        MCH_DEBUG("{} is suitable", properties.deviceName)
        physical_device = device;
        MCH_INFO("Select Device {}", properties.deviceName)

        auto memoryProperties = physical_device.getMemoryProperties();
        MCH_DEBUG("\tDevice Driver Version: {}.{}.{}", VK_VERSION_MAJOR(properties.driverVersion), VK_VERSION_MINOR(properties.driverVersion), VK_VERSION_PATCH(properties.driverVersion))
        MCH_DEBUG("\tVulkan API Version: {}.{}.{}", VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion))
        for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++) {
            float size = ((float)memoryProperties.memoryHeaps[i].size) / 1024.0f / 1024.0f / 1024.0f;
            if (memoryProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
                MCH_DEBUG("\tLocal GPU Memory: {} Gib", size)
            }
            else {
                MCH_DEBUG("\tShared GPU Memory: {} Gib", size)
            }
        }
        return true;
    }

}
