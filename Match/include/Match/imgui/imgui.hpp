#pragma once

#include <Match/vulkan/renderer.hpp>

namespace Match {
    class MATCH_API ImGuiLayer final : public RenderLayer {
        no_copy_move_construction(ImGuiLayer)
    public:
        ImGuiLayer(Renderer &renderer, const std::vector<std::string> &input_attachments = {});
        void begin_render() override;
        void end_render() override;
        ~ImGuiLayer();
    INNER_VISIBLE:
        vk::DescriptorPool descriptor_pool;
    };
}