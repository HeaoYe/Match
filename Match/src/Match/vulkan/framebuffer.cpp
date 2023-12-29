#include <Match/vulkan/framebuffer.hpp>
#include <Match/vulkan/renderer.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/constant.hpp>
#include "inner.hpp"

namespace Match {
    Attachment::Attachment() {}

    Attachment::Attachment(const VkAttachmentDescription& description, VkImageUsageFlags usage, VkImageAspectFlags aspect) {
        image = std::make_unique<Image>(runtime_setting->window_size.width, runtime_setting->window_size.height, description.format, usage, description.samples, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        image_view = create_image_view(image->image, description.format, aspect, 1);
    }

    Attachment::Attachment(Attachment &&rhs) {
        image = std::move(rhs.image);
        image_view = rhs.image_view;
        rhs.image_view = VK_NULL_HANDLE;
    }

    void Attachment::operator=(Attachment &&rhs) {
        image = std::move(rhs.image);
        image_view = rhs.image_view;
        rhs.image_view = VK_NULL_HANDLE;
    }

    Attachment::~Attachment() {
        if (image.get() != nullptr) {
            vkDestroyImageView(manager->device->device, image_view, manager->allocator);
            image.reset();
        }
    }

    FrameBuffer::FrameBuffer(const Renderer &renderer, const std::vector<VkImageView> &image_views) {
        VkFramebufferCreateInfo framebuffer_create_info { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_create_info.renderPass = renderer.render_pass->render_pass;
        framebuffer_create_info.width = runtime_setting->get_window_size().width;
        framebuffer_create_info.height = runtime_setting->get_window_size().height;
        framebuffer_create_info.layers = 1;
        framebuffer_create_info.attachmentCount = image_views.size();
        framebuffer_create_info.pAttachments = image_views.data();

        vk_assert(vkCreateFramebuffer(manager->device->device, &framebuffer_create_info, manager->allocator, &framebuffer));
    }

    FrameBuffer::~FrameBuffer() {
        vkDestroyFramebuffer(manager->device->device, framebuffer, manager->allocator);
    }

    FrameBufferSet::FrameBufferSet(const Renderer &renderer) {
        uint32_t attachments_count = renderer.render_pass_builder->final_attachments.size();
        std::vector<VkImageView> image_views(attachments_count);
        attachments.resize(attachments_count);
        uint32_t swapchain_image_view_idx;

        for (auto &[name, idx] : renderer.render_pass_builder->attachments_map) {
            auto attachment = renderer.render_pass_builder->attachments[idx];
            if (idx == 0) {
                if (attachment.description_read.has_value()) {
                    attachments[idx] = std::move(Attachment(attachment.description_write, attachment.usage, attachment.aspect));
                    swapchain_image_view_idx = renderer.render_pass_builder->get_attachment_index(SWAPCHAIN_IMAGE_ATTACHMENT, true);
                } else {
                    swapchain_image_view_idx = 0;
                }
                continue;
            }
            attachments[idx] = std::move(Attachment(attachment.description_write, attachment.usage, attachment.aspect));
            if (attachment.description_read.has_value()) {
                attachments[renderer.render_pass_builder->get_attachment_index(name, true)] = std::move(Attachment(attachment.description_read.value(), attachment.usage_read, attachment.aspect));
            }
        }
        for (uint32_t idx = 0; idx < attachments_count; idx ++) {
            image_views[idx] = attachments[idx].image_view;
        }

        for (uint32_t i = 0; i < manager->swapchain->image_count; i ++) {
            image_views[swapchain_image_view_idx] = manager->swapchain->image_views[i];
            framebuffers.push_back(std::make_unique<FrameBuffer>(renderer, image_views));
        }
    }

    FrameBufferSet::~FrameBufferSet() {
        framebuffers.clear();
        attachments.clear();
    }
}
