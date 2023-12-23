#pragma once

#include <Match/vulkan/renderer.hpp>

namespace Match {
    class ImGuiLayer final : public RenderLayer {
        no_copy_move_construction(ImGuiLayer)
    public:
        ImGuiLayer(Renderer &renderer);
        void begin_render() override;
        void end_render() override;
        ~ImGuiLayer();
    INNER_VISIBLE:
        VkDescriptorPool descriptor_pool;
    };
}