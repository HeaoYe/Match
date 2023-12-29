#pragma once

#include <Match/vulkan/resource/renderpass_builder.hpp>

namespace Match {

    class RenderPass {
        no_copy_move_construction(RenderPass)
    public:
        RenderPass(std::shared_ptr<RenderPassBuilder> builder);
        ~RenderPass();
    INNER_VISIBLE:
        VkRenderPass render_pass;
    };
}
