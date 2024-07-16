#pragma once

#include <Match/vulkan/renderer.hpp>

namespace Match {
    class ImGuiLayer final : public RenderLayer {
        no_copy_move_construction(ImGuiLayer)
    public:
        MATCH_API ImGuiLayer(Renderer &renderer, const std::vector<std::string> &input_attachments = {});
        MATCH_API void begin_render() override;
        MATCH_API void end_render() override;
        MATCH_API ~ImGuiLayer();
    INNER_VISIBLE:
        vk::DescriptorPool descriptor_pool;
    };
}
