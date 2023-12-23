#include <Match/imgui/imgui.hpp>
#include <Match/core/window.hpp>
#include <Match/constant.hpp>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include "../vulkan/inner.hpp"

namespace Match {
    ImGuiLayer::ImGuiLayer(Renderer &renderer) : RenderLayer(renderer) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.DisplaySize.x = static_cast<float>(runtime_setting->window_size.width);
        io.DisplaySize.y = static_cast<float>(runtime_setting->window_size.height);
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        ImGui::StyleColorsDark();
        auto &style = ImGui::GetStyle();
        style.WindowMinSize = { 160, 160 };
        style.WindowRounding = 2;

        std::vector<VkDescriptorPoolSize> pool_sizes = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        create_info.maxSets = pool_sizes.size() * 1000;
        create_info.poolSizeCount = pool_sizes.size();
        create_info.pPoolSizes = pool_sizes.data();
        vkCreateDescriptorPool(manager->device->device, &create_info, manager->allocator, &descriptor_pool);

        auto &last_subpass_name = renderer.render_pass_builder->subpass_builders.back()->name;
        auto &subpass = renderer.render_pass_builder->add_subpass("ImGui Layer");
        subpass.attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        subpass.wait_for(last_subpass_name, { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT }, { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT });
        renderer.update_renderpass();

        ImGui_ImplGlfw_InitForVulkan(window->window, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = manager->instance;
        init_info.PhysicalDevice = manager->device->physical_device;
        init_info.Device = manager->device->device;
        init_info.QueueFamily = manager->device->graphics_family_index;
        init_info.Queue = manager->device->graphics_queue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = descriptor_pool;
        init_info.Subpass = renderer.render_pass_builder->subpass_builders.size() - 1;
        init_info.MinImageCount = manager->swapchain->image_count;
        init_info.ImageCount = manager->swapchain->image_count;
        init_info.MSAASamples = runtime_setting->get_multisample_count();
        init_info.Allocator = manager->allocator;
        init_info.CheckVkResultFn = [](VkResult res) {
            if (res != VK_SUCCESS) {
                MCH_ERROR("ImGui Vulkan Error: {}", res)
            }
        };
        ImGui_ImplVulkan_Init(&init_info, renderer.render_pass->render_pass);
    }

    void ImGuiLayer::begin_render() {
        renderer.continue_subpass_to("ImGui Layer");
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::end_render() {
        ImGui::EndFrame();
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(draw_data, renderer.current_buffer);
    }

    ImGuiLayer::~ImGuiLayer() {
        vkDeviceWaitIdle(manager->device->device);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        vkDestroyDescriptorPool(manager->device->device, descriptor_pool, manager->allocator);
        ImGui::DestroyContext();
    }
}
