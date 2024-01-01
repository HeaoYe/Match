#include <Match/vulkan/renderpass.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/constant.hpp>
#include "inner.hpp"

namespace Match {
    RenderPass::RenderPass(std::shared_ptr<RenderPassBuilder> builder) {
        auto create_info = builder->build();
        render_pass = manager->device->device.createRenderPass(create_info);
    }

    RenderPass::~RenderPass() {
        manager->device->device.destroyRenderPass(render_pass);
    }
}
