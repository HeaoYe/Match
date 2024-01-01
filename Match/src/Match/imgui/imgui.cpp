#include <Match/imgui/imgui.hpp>
#include <Match/core/window.hpp>
#include <Match/core/setting.hpp>
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
        ImGui::StyleColorsClassic();
        auto &style = ImGui::GetStyle();
        style.WindowMinSize = { 160, 160 };
        style.WindowRounding = 2;

        std::vector<vk::DescriptorPoolSize> pool_sizes = {
            { vk::DescriptorType::eSampler, 1000 },
            { vk::DescriptorType::eCombinedImageSampler, 1000 },
            { vk::DescriptorType::eSampledImage, 1000 },
            { vk::DescriptorType::eStorageImage, 1000 },
            { vk::DescriptorType::eUniformTexelBuffer, 1000 },
            { vk::DescriptorType::eStorageTexelBuffer, 1000 },
            { vk::DescriptorType::eUniformBuffer, 1000 },
            { vk::DescriptorType::eStorageBuffer, 1000 },
            { vk::DescriptorType::eUniformBufferDynamic, 1000 },
            { vk::DescriptorType::eStorageBufferDynamic, 1000 },
            { vk::DescriptorType::eInputAttachment, 1000 }
        };
        vk::DescriptorPoolCreateInfo create_info {};
        create_info.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
            .setMaxSets(pool_sizes.size() * 1000)
            .setPoolSizes(pool_sizes);
        descriptor_pool = manager->device->device.createDescriptorPool(create_info);

        auto &last_subpass_name = renderer.render_pass_builder->subpass_builders.back()->name;
        renderer.render_pass_builder->add_subpass("ImGui Layer")
            .attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT)
            .wait_for(
                last_subpass_name,
                { vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite },
                { vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite }
            );
        renderer.update_renderpass();

        std::vector<ImFontConfig> font_configs;
        if (setting.default_font_filename.empty() && setting.chinese_font_filename.empty()) {
            auto &font_config = font_configs.emplace_back();
            font_config.SizePixels = setting.font_size;
            io.Fonts->AddFontDefault(&font_config);
        }
        if (!setting.default_font_filename.empty()) {
            auto &font_config = font_configs.emplace_back();
            font_config.GlyphExtraSpacing = { 0.25f, 0.0f };
            font_config.RasterizerMultiply = 1.2f;
            font_config.MergeMode = font_configs.size() != 1;
            io.Fonts->AddFontFromFileTTF(setting.default_font_filename.c_str(), setting.font_size, &font_config, io.Fonts->GetGlyphRangesDefault());
        }
        if (!setting.chinese_font_filename.empty()) {
            auto &font_config = font_configs.emplace_back();
            font_config.GlyphExtraSpacing = { 1.0f, 0.0f };
            font_config.RasterizerMultiply = 1.2f;
            font_config.MergeMode = font_configs.size() != 1;
            io.Fonts->AddFontFromFileTTF(setting.chinese_font_filename.c_str(), setting.font_size, &font_config, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        }
        io.Fonts->Build();

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
        init_info.MSAASamples = static_cast<VkSampleCountFlagBits>(runtime_setting->multisample_count);
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = [](VkResult res) {
            if (res != VK_SUCCESS) {
                MCH_ERROR("ImGui Vulkan Error: {}", static_cast<uint32_t>(res))
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
        manager->device->device.waitIdle();
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        manager->device->device.destroyDescriptorPool(descriptor_pool);
        ImGui::DestroyContext();
    }
}
