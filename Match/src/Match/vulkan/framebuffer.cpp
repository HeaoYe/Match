#include <Match/vulkan/renderer.hpp>
#include <Match/vulkan/framebuffer.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/constant.hpp>
#include "inner.hpp"

namespace Match {
    Attachment::Attachment() : has_image(false) {}

    Attachment::Attachment(const VkAttachmentDescription& description, VkImageUsageFlags usage, VkImageAspectFlags aspect) : has_image(true) {
        image = std::make_unique<Image>(runtime_setting->window_size.width, runtime_setting->window_size.height, description.format, usage, description.samples, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        transition_image_layout(image->image, aspect, { VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT }, { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT });
        image_view = create_image_view(image->image, description.format, aspect, 1);
    }

    Attachment::Attachment(VkImageView image_view) : has_image(false), image_view(image_view) {}

    Attachment::Attachment(Attachment &&rhs) {
        has_image = rhs.has_image;
        if (rhs.has_image) {
            image = std::move(rhs.image);
        }
        image_view = rhs.image_view;
        rhs.image_view = VK_NULL_HANDLE;
    }

    void Attachment::operator=(Attachment &&rhs) {
        has_image = rhs.has_image;
        if (rhs.has_image) {
            image = std::move(rhs.image);
        }
        image_view = rhs.image_view;
        rhs.image_view = VK_NULL_HANDLE;
    }

    Attachment::~Attachment() {
        if (has_image) {
            vkDestroyImageView(manager->device->device, image_view, manager->allocator);
            image.reset();
        }
    }

    FrameBuffer::FrameBuffer(const Renderer &renderer, uint32_t index) {
        std::vector<VkImageView> image_views(renderer.render_pass_builder->attachments.size());
        attachments.resize(renderer.render_pass_builder->attachments.size());

        for (auto [name, idx] : renderer.render_pass_builder->attachments_map) {
            auto info = renderer.render_pass_builder->attachment_infos[idx];
            if (name != SWAPCHAIN_IMAGE_ATTACHMENT) {
                attachments[idx] = std::move(Attachment(renderer.render_pass_builder->attachments[idx], info.first, info.second));
            } else {
                attachments[idx] = std::move(Attachment(manager->swapchain->image_views[index]));
            }
            image_views[idx] = attachments[idx].image_view;
        }

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
        attachments.clear();
    }

    FrameBufferSet::FrameBufferSet(const Renderer &renderer) {
        for (uint32_t i = 0; i < manager->swapchain->image_count; i ++) {
            framebuffers.push_back(std::make_unique<FrameBuffer>(renderer, i));
        }
    }

    FrameBufferSet::~FrameBufferSet() {
        framebuffers.clear();
    }
}
