#include <Match/vulkan/manager.hpp>
#include <Match/core/window.hpp>
#include <Match/core/setting.hpp>
#include "inner.hpp"

namespace Match {
    std::unique_ptr<APIManager> APIManager::manager;

    APIManager &APIManager::GetInstance() {
        if (manager.get() == nullptr) {
            manager.reset(new APIManager);
        }
        return *manager;
    }

    void APIManager::Quit() {
        manager.reset();
    }

    APIManager::APIManager() {
        create_vk_instance();
        if (setting.enable_ray_tracing) {
            vk::DynamicLoader dl;
            auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
            dispatcher = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr);
        }

        this->runtime_setting = std::make_shared<RuntimeSetting>();
        ::Match::manager = this;
    }

    APIManager::~APIManager() {
        ::Match::manager = nullptr;
    }

    std::vector<std::string> APIManager::enumerate_devices() {
        return device->enumerate_devices();
    }

    std::shared_ptr<RuntimeSetting> APIManager::get_runtime_setting() {
        return runtime_setting;
    }

    std::shared_ptr<ResourceFactory> APIManager::create_resource_factory(const std::string &root) {
        return std::make_shared<ResourceFactory>(root);
    }

    CommandPool &APIManager::get_command_pool() {
        return *command_pool;
    }

    void APIManager::initialize() {
        MCH_INFO("Initialize Vulkan API")
        create_vk_surface();
        device = std::make_unique<Device>();
        initialize_vma();
        swapchain = std::make_unique<Swapchain>();
        command_pool = std::make_unique<CommandPool>(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        descriptor_pool = std::make_unique<DescriptorPool>();
    }

    void APIManager::create_vk_instance() {
        vk::ApplicationInfo app_info {};
        app_info.setPApplicationName(setting.app_name.c_str())
            .setApplicationVersion(setting.app_version)
            .setPEngineName(setting.engine_name.c_str())
            .setEngineVersion(setting.engine_version)
            .setApiVersion(vk::ApiVersion13);
        vk::InstanceCreateInfo instance_create_info {};
        instance_create_info.setPApplicationInfo(&app_info);

        std::map<std::string, bool> required_extensions;
        uint32_t count;
        const char **glfw_required_extensions = glfwGetRequiredInstanceExtensions(&count);
        for (uint32_t i = 0; i < count; i ++) {
            required_extensions.insert(std::make_pair(std::string(glfw_required_extensions[i]), false));
        }

        std::vector<const char *> enabled_extensions;
        auto extensions = vk::enumerateInstanceExtensionProperties();
        bool found = false;
        for (auto &extension : extensions) {
            for (auto &required_extension : required_extensions) {
                if (required_extension.first == extension.extensionName) {
                    required_extension.second = true;
                    enabled_extensions.push_back(required_extension.first.c_str());
                    break;
                }
            }
            MCH_TRACE("EXT: {}-{}", extension.extensionName, extension.specVersion)
        }

        for (auto &required_extension : required_extensions) {
            if (!required_extension.second) {
                MCH_ERROR("Faild find required vulkan extension: {}", required_extension.first)
            }
        }

        instance_create_info.setPEnabledExtensionNames(enabled_extensions);

        const char *validation_layers[] = { "VK_LAYER_KHRONOS_validation" };
        if (setting.debug_mode) {
            auto layers = vk::enumerateInstanceLayerProperties();
            bool found = false;
            for (auto &layer : layers) {
                if (strcmp(validation_layers[0], layer.layerName) == 0) {
                    found = true;
                    instance_create_info.setPEnabledLayerNames(validation_layers);
                }
                MCH_TRACE("LYR: {} \"{}\" {}-{}", layer.layerName, layer.description, layer.implementationVersion, layer.specVersion)
            }
            if (found == false) {
                MCH_ERROR("Cannot find Vulkan Validation Layer")
            }
        }

        instance = vk::createInstance(instance_create_info);
    }

    void APIManager::create_vk_surface() {
        VkSurfaceKHR surface;
        glfwCreateWindowSurface(instance, window->get_glfw_window(), nullptr, &surface);
        this->surface = surface;
    }

    void APIManager::initialize_vma() {
        VmaAllocatorCreateInfo create_info {};
        create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        create_info.vulkanApiVersion = VK_API_VERSION_1_3;
        create_info.instance = instance;
        create_info.physicalDevice = device->physical_device;
        create_info.device = device->device;
        vmaCreateAllocator(&create_info, &vma_allocator);
    }

    void APIManager::recreate_swapchin() {
        vkDeviceWaitIdle(device->device);
        swapchain.reset();
        swapchain = std::make_unique<Swapchain>();
    }

    void APIManager::destroy() {
        MCH_INFO("Destroy Vulkan API")
        descriptor_pool.reset();
        command_pool.reset();
        swapchain.reset();
        vmaDestroyAllocator(vma_allocator);
        device.reset();
        this->runtime_setting.reset();
        instance.destroySurfaceKHR(surface);
        instance.destroy();
    }
}
