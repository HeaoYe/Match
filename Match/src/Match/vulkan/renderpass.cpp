#include <Match/vulkan/renderpass.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/constant.hpp>
#include "inner.hpp"

namespace Match {
    RenderPass::RenderPass(std::shared_ptr<RenderPassBuilder> builder) {
        auto create_info = builder->build();
        vk_assert(vkCreateRenderPass(manager->device->device, &create_info, manager->allocator, &render_pass));
    }

    RenderPass::~RenderPass() {
        vkDestroyRenderPass(manager->device->device, render_pass, manager->allocator);
    }
}
