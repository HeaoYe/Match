#include <Match/vulkan/device.hpp>
#include <Match/core/setting.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/constant.hpp>
#include "inner.hpp"

namespace Match {
    std::vector<std::string> Device::enumerate_devices() const {
        auto devices = manager->instance.enumeratePhysicalDevices();
        std::vector<std::string> device_names;
        vk::PhysicalDeviceProperties properties;
        auto idx = 0;
        for (auto &device : devices) {
            properties = device.getProperties();
            MCH_WARN("No.{} \"{}\"", idx, std::string(properties.deviceName))
            idx ++;
        }
        return device_names;
    }

    Device::Device() {
        bool selected_device = false;
        if (setting.device_name != AUTO_SELECT_DEVICE) {
            auto devices = manager->instance.enumeratePhysicalDevices();
            vk::PhysicalDeviceProperties properties;
            bool found_device = false;
            for (auto &device : devices) {
                properties = device.getProperties();
                if (properties.deviceName.data() == setting.device_name) {
                    MCH_DEBUG("Found device {}", std::string(properties.deviceName))
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
        if ((setting.device_name == AUTO_SELECT_DEVICE) || (!selected_device)) {
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
        features.shaderInt64 = VK_TRUE;
        features.fragmentStoresAndAtomics = VK_TRUE;
        features.multiDrawIndirect = VK_TRUE;
        features.geometryShader = VK_TRUE;
        features.fillModeNonSolid = VK_TRUE;
        features.wideLines = VK_TRUE;
        vk::PhysicalDeviceVulkan12Features vk12_features {};
        vk12_features.runtimeDescriptorArray = VK_TRUE;
        vk12_features.bufferDeviceAddress = VK_TRUE;
        vk12_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        vk12_features.descriptorBindingVariableDescriptorCount = VK_TRUE;
        vk12_features.drawIndirectCount = VK_TRUE;
        vk12_features.samplerFilterMinmax = VK_TRUE;
        vk::DeviceCreateInfo device_create_info {};
        device_create_info.setPNext(&vk12_features);

        std::map<std::string, bool> required_extensions = {
            { VK_KHR_SWAPCHAIN_EXTENSION_NAME, false },
            { VK_KHR_BIND_MEMORY_2_EXTENSION_NAME, false }
        };

        vk::PhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features {};
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline_features {};
        vk::PhysicalDeviceRayQueryFeaturesKHR ray_query_features {};
        if (setting.enable_ray_tracing) {
            required_extensions.insert(std::make_pair(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, false));
            required_extensions.insert(std::make_pair(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, false));
            required_extensions.insert(std::make_pair(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, false));
            required_extensions.insert(std::make_pair(VK_KHR_RAY_QUERY_EXTENSION_NAME, false));

            acceleration_structure_features.setAccelerationStructure(VK_TRUE)
                .setAccelerationStructureCaptureReplay(VK_TRUE)
                .setAccelerationStructureIndirectBuild(VK_FALSE)
                .setAccelerationStructureHostCommands(VK_FALSE)
                .setDescriptorBindingAccelerationStructureUpdateAfterBind(VK_TRUE);
            ray_tracing_pipeline_features.setRayTracingPipeline(VK_TRUE)
                .setRayTracingPipelineShaderGroupHandleCaptureReplay(VK_FALSE)
                .setRayTracingPipelineShaderGroupHandleCaptureReplayMixed(VK_FALSE)
                .setRayTracingPipelineTraceRaysIndirect(VK_TRUE)
                .setRayTraversalPrimitiveCulling(VK_TRUE);
            ray_query_features.setRayQuery(VK_TRUE);

            device_create_info.setPNext(
                &acceleration_structure_features.setPNext(
                    &ray_tracing_pipeline_features.setPNext(
                        &ray_query_features.setPNext(&vk12_features)
                    )
                )
            );
        }

        for (const auto &extension : setting.device_extensions) {
            required_extensions.insert(std::make_pair(extension, false));
        }

        std::vector<const char *> enabled_extensions;
        auto extensions = physical_device.enumerateDeviceExtensionProperties();
        bool found = false;
        for (auto &extension : extensions) {
            for (auto &required_extension : required_extensions) {
                if (required_extension.first == extension.extensionName) {
                    required_extension.second = true;
                    enabled_extensions.push_back(required_extension.first.c_str());
                    break;
                }
            }
            MCH_TRACE("PHY EXT: {}-{}", std::string(extension.extensionName), extension.specVersion)
        }

        for (auto &required_extension : required_extensions) {
            if (!required_extension.second) {
                MCH_ERROR("Faild find required vulkan physical device extension: {}", required_extension.first)
            }
        }

        const char *layers[] = { "VK_LAYER_KHRONOS_validation" };
        device_create_info.setQueueCreateInfos(queue_create_infos)
            .setPEnabledExtensionNames(enabled_extensions)
            .setPEnabledFeatures(&features);
        if (setting.debug_mode) {
            device_create_info.setPEnabledLayerNames(layers);
        }
        device = physical_device.createDevice(device_create_info);

        graphics_queue = device.getQueue(graphics_family_index, 0);
        present_queue = device.getQueue(present_family_index, 0);
        transfer_queue = device.getQueue(transfer_family_index, 0);
        compute_queue = device.getQueue(compute_family_index, 0);

        if (setting.enable_ray_tracing) {
            vk::PhysicalDeviceProperties2 properties {};
            vk::PhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_properties {};
            vk::PhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties {};
            properties.pNext = &acceleration_structure_properties;
            acceleration_structure_properties.pNext = &ray_tracing_pipeline_properties;
            physical_device.getProperties2(&properties);
            MCH_TRACE("Max Ray Recursion Depth: {}", ray_tracing_pipeline_properties.maxRayRecursionDepth);
        }
    }

    Device::~Device() {
        device.destroy();
    }

    bool Device::check_device_suitable(vk::PhysicalDevice device) {
        auto properties = device.getProperties();
        auto features = device.getFeatures();

        if (properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
            MCH_DEBUG("{} is not a discrete gpu -- skipping", std::string(properties.deviceName))
            return false;
        }

        if (!features.samplerAnisotropy) {
            MCH_DEBUG("{} does not support sampler anisotropy -- skipping", std::string(properties.deviceName))
            return false;
        }

        if (!features.sampleRateShading) {
            MCH_DEBUG("{} does not support sampler rate shading -- skipping", std::string(properties.deviceName))
            return false;
        }

        if (!features.shaderInt64) {
            MCH_DEBUG("{} does not support type int 64 -- warning", std::string(properties.deviceName))
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
            MCH_DEBUG("{} has no suitable queue family -- skipping", std::string(properties.deviceName))
            return false;
        }

        MCH_DEBUG("{} is suitable", std::string(properties.deviceName))
        physical_device = device;
        MCH_INFO("Select Device {}", std::string(properties.deviceName))

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
