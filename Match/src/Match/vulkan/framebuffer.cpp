#include <Match/vulkan/framebuffer.hpp>
#include "inner.hpp"

namespace Match {
    FrameBuffer::FrameBuffer(uint32_t index) {
        std::vector<VkImageView> attachments = {
            manager->swapchain->image_views[index]
        };
        VkFramebufferCreateInfo framebuffer_create_info { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_create_info.renderPass = manager->render_pass->render_pass;
        framebuffer_create_info.width = runtime_setting->get_window_size().width;
        framebuffer_create_info.height = runtime_setting->get_window_size().height;
        framebuffer_create_info.layers = 1;
        framebuffer_create_info.attachmentCount = attachments.size();
        framebuffer_create_info.pAttachments = attachments.data();

        vk_assert(vkCreateFramebuffer(manager->device->device, &framebuffer_create_info, manager->allocator, &framebuffer));
    }

    FrameBuffer::~FrameBuffer() {
        vkDestroyFramebuffer(manager->device->device, framebuffer, manager->allocator);
    }

    FrameBufferSet::FrameBufferSet() {
        for (uint32_t i = 0; i < manager->swapchain->image_count; i ++) {
            framebuffers.push_back(std::make_unique<FrameBuffer>(i));
        }
    }

    FrameBufferSet::~FrameBufferSet() {
        framebuffers.clear();
    }
}
