#pragma once

#include <Match/vulkan/commons.hpp>
#include <Match/vulkan/device.hpp>
#include <Match/vulkan/swapchain.hpp>
#include <Match/core/setting.hpp>
#include <Match/vulkan/resource/resource_factory.hpp>
#include <Match/vulkan/renderpass.hpp>
#include <Match/vulkan/framebuffer.hpp>
#include <Match/vulkan/command_pool.hpp>

namespace Match {
    struct APIInfo;
    
    class APIManager {
        no_copy_move_construction(APIManager)
    private:
        APIManager();
    public:
        ~APIManager();
        std::vector<std::string> enumerate_devices();
        void initialize();
        RenderPassBuilder &get_render_pass_builder();
        void build_render_pass();
        std::shared_ptr<RuntimeSetting> get_runtime_setting();
        std::shared_ptr<ResourceFactory> create_resource_factory(const std::string &root);
        CommandPool &get_command_pool();
        void destroy();
    INNER_VISIBLE:
        static APIInfo &CreateAPIInfo();
        static APIManager &GetInstance();
        static void Quit();
    INNER_VISIBLE:
        void recreate_swapchin();
    private:
        void create_vk_instance();
        void create_vk_surface();
        void initialize_vma();
    private:
        static std::unique_ptr<APIManager> manager;
        static std::unique_ptr<APIInfo> info;
    INNER_VISIBLE:
        VkAllocationCallbacks* allocator;
        VmaAllocator vma_allocator;
        VkInstance instance;
        VkSurfaceKHR surface;
        std::shared_ptr<RuntimeSetting> runtime_setting;
        std::unique_ptr<Device> device;
        std::unique_ptr<Swapchain> swapchain;
        std::unique_ptr<CommandPool> command_pool;
        std::unique_ptr<RenderPassBuilder> render_pass_builder;
        std::unique_ptr<RenderPass> render_pass;
        std::unique_ptr<FrameBufferSet> framebuffer_set;
    };
}
