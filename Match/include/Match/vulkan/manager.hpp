#pragma once

#include <Match/vulkan/commons.hpp>
#include <Match/vulkan/device.hpp>
#include <Match/vulkan/swapchain.hpp>
#include <Match/core/setting.hpp>
#include <Match/vulkan/resource/resource_factory.hpp>
#include <Match/vulkan/command_pool.hpp>
#include <Match/vulkan/descriptor_resource/descriptor_pool.hpp>

namespace Match {
    class APIManager {
        no_copy_move_construction(APIManager)
        friend MATCH_API std::vector<std::string> EnumerateDevices();
        friend MATCH_API APIManager &Initialize();
        friend MATCH_API void Destroy();
        friend Renderer;
    private:
        MATCH_API APIManager();
    public:
        MATCH_API ~APIManager();
        MATCH_API std::vector<std::string> enumerate_devices();
        MATCH_API void initialize();
        MATCH_API std::shared_ptr<RuntimeSetting> get_runtime_setting();
        MATCH_API std::shared_ptr<ResourceFactory> create_resource_factory(const std::string &root);
        MATCH_API CommandPool &get_command_pool();
        MATCH_API void destroy();
    private:
        MATCH_API static APIManager &GetInstance();
        MATCH_API static void Quit();
    private:
        MATCH_API void recreate_swapchin();
    private:
        MATCH_API void create_vk_instance();
        MATCH_API void create_vk_surface();
        MATCH_API void initialize_vma();
    private:
        MATCH_API static std::unique_ptr<APIManager> manager;
    INNER_VISIBLE:
        vk::DispatchLoaderDynamic dispatcher;
        VmaAllocator vma_allocator;
        vk::Instance instance;
        vk::SurfaceKHR surface;
        std::shared_ptr<RuntimeSetting> runtime_setting;
        std::unique_ptr<Device> device;
        std::unique_ptr<Swapchain> swapchain;
        std::unique_ptr<CommandPool> command_pool;
        std::unique_ptr<DescriptorPool> descriptor_pool;
    };
}
