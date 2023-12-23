#include <Match/vulkan/manager.hpp>
#include <Match/core/window.hpp>
#include <Match/core/setting.hpp>
#include "inner.hpp"
#include <sstream>

#if defined (MATCH_WAYLAND)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#include <vulkan/vulkan_wayland.h>
#endif
#if defined (MATCH_XCB)
#include <vulkan/vulkan_xcb.h>
#endif
#if defined (MATCH_XLIB)
#define GLFW_EXPOSE_NATIVE_X11
#include <vulkan/vulkan_xlib.h>
#endif
#if defined (MATCH_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <vulkan/vulkan_win32.h>
#endif
#include <GLFW/glfw3native.h>

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
        allocator = nullptr;
        create_vk_instance();
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

    std::shared_ptr<Renderer> APIManager::create_renderer(std::shared_ptr<RenderPassBuilder> builder) {
        return std::make_shared<Renderer>(builder);
    }

    CommandPool &APIManager::get_command_pool() {
        return *command_pool;
    }

    void APIManager::initialize(const APIInfo &info) {
        MCH_INFO("Initialize Vulkan API")
        create_vk_surface(info);
        device = std::make_unique<Device>();
        initialize_vma();
        swapchain = std::make_unique<Swapchain>();
        command_pool = std::make_unique<CommandPool>(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        descriptor_pool = std::make_unique<DescriptorPool>();
    }

    void APIManager::create_vk_instance() {
        VkApplicationInfo app_info { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.pApplicationName = setting.app_name.c_str();
        app_info.applicationVersion = setting.app_version;
        app_info.pEngineName = setting.engine_name.c_str();
        app_info.engineVersion = setting.engine_version;
        app_info.apiVersion = VK_API_VERSION_1_3;
        VkInstanceCreateInfo instance_create_info { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instance_create_info.pApplicationInfo = &app_info;

        std::stringstream ss;
        ss << "VK_KHR_";

        switch (setting.render_backend) {
#if defined (MATCH_WAYLAND)
        case PlatformWindowSystem::eWayland:
            ss << "wayland";
            break;
#endif
#if defined (MATCH_XLIB)
        case PlatformWindowSystem::eXlib:
            ss << "xlib";
            break;
#endif
#if defined (MATCH_XCB)
        case PlatformWindowSystem::eXcb:
            ss << "xcb";
            break;
#endif
#if defined (MATCH_WIN32)
        case PlatformWindowSystem::eWin32:
            ss << "win32";
            break;
#endif
        default:
            MCH_FATAL("Unknown platform render backend, please set setting.render_backend")
            return;
        }
        ss << "_surface";
        std::map<std::string, bool> required_extensions = {
            { "VK_KHR_surface", false },
            { ss.str(), false },
        };

        std::vector<const char *> enabled_extensions;
        uint32_t extension_count = 0;
        bool found = false;
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> extensions(extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

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

        instance_create_info.enabledExtensionCount = enabled_extensions.size();
        instance_create_info.ppEnabledExtensionNames = enabled_extensions.data();

        const char *validation_layers[] = { "VK_LAYER_KHRONOS_validation" };
        if (setting.debug_mode) {
            uint32_t layer_count;
            vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
            std::vector<VkLayerProperties> layers(layer_count);
            vkEnumerateInstanceLayerProperties(&layer_count, layers.data());
            bool found = false;
            for (auto &layer : layers) {
                if (strcmp(validation_layers[0], layer.layerName) == 0) {
                    found = true;
                    instance_create_info.enabledLayerCount = 1;
                    instance_create_info.ppEnabledLayerNames = validation_layers;
                }
                MCH_TRACE("LYR: {} \"{}\" {}-{}", layer.layerName, layer.description, layer.implementationVersion, layer.specVersion)
            }
            if (found == false) {
                MCH_ERROR("Cannot find Vulkan Validation Layer")
            }
        }

        vk_assert(vkCreateInstance(&instance_create_info, allocator, &instance));
    }

    void APIManager::create_vk_surface(const APIInfo &info) {
        switch (setting.render_backend) {
#if defined (MATCH_WAYLAND)
        case PlatformWindowSystem::eWayland: {
            VkWaylandSurfaceCreateInfoKHR wayland_surface_create_info { VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR };
            wayland_surface_create_info.surface = glfwGetWaylandWindow(window->window);
            wayland_surface_create_info.display = glfwGetWaylandDisplay();
            vk_check(vkCreateWaylandSurfaceKHR(instance, &wayland_surface_create_info, allocator, &surface));
            break;
        }
#endif
#if defined (MATCH_XLIB)
        case PlatformWindowSystem::eXlib: {
            VkXlibSurfaceCreateInfoKHR xlib_surface_create_info { VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR };
            xlib_surface_create_info.dpy = glfwGetX11Display();
            xlib_surface_create_info.window = glfwGetX11Window(window->window);
            vk_check(vkCreateXlibSurfaceKHR(instance, &xlib_surface_create_info, allocator, &surface));
            break;
        }
#endif
#if defined (MATCH_XCB)
        case PlatformWindowSystem::eXcb: {
            VkXcbSurfaceCreateInfoKHR xcb_surface_create_info { VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR };
            xcb_surface_create_info.connection = info.xcb_connection;
            xcb_surface_create_info.window = info.xcb_window;
            vk_check(vkCreateXcbSurfaceKHR(instance, &xcb_surface_create_info, allocator, &surface));
            break;
        }
#endif
#if defined (MATCH_WIN32)
        case PlatformWindowSystem::eWin32: {
            VkWin32SurfaceCreateInfoKHR win32_surface_create_info { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
            win32_surface_create_info.hinstance = info.win32_hinstance;
            win32_surface_create_info.hwnd = glfwGetWin32Window(window->window);
            vk_check(vkCreateWin32SurfaceKHR(instance, &win32_surface_create_info, allocator, &surface));
            break;
        }
#endif
        default:
            MCH_FATAL("Unknown platform render backend, please set setting.render_backend")
            return;
        }
    }

    void APIManager::initialize_vma() {
        VmaAllocatorCreateInfo create_info {};
        create_info.flags = 0;
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
        vkDestroySurfaceKHR(instance, surface, allocator);
        vkDestroyInstance(instance, allocator);
    }
}
