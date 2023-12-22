#pragma once

#include <Match/vulkan/commons.hpp>
#include <Match/vulkan/resource/image.hpp>

namespace Match {
    class Renderer;

    class Attachment {
        no_copy_construction(Attachment)
    public:
        Attachment();
        Attachment(const VkAttachmentDescription& description, VkImageUsageFlags usage, VkImageAspectFlags aspect);
        Attachment(VkImageView image_view);
        Attachment(Attachment &&rhs);
        void operator=(Attachment &&rhs);
        ~Attachment();
    INNER_VISIBLE:
        bool has_image;
        std::unique_ptr<Image> image;
        VkImageView image_view;
    };

    class FrameBuffer {
        no_copy_move_construction(FrameBuffer)
    public:
        FrameBuffer(const Renderer &renderer, uint32_t index);
        ~FrameBuffer();
    INNER_VISIBLE:
        std::vector<Attachment> attachments;
        VkFramebuffer framebuffer;
    };

    class FrameBufferSet {
        no_copy_move_construction(FrameBufferSet)
    public:
        FrameBufferSet(const Renderer &renderer);
        ~FrameBufferSet();
    INNER_VISIBLE:
        std::vector<std::unique_ptr<FrameBuffer>> framebuffers;
    };
}
