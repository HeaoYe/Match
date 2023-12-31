#pragma once

#include <Match/vulkan/commons.hpp>
#include <Match/vulkan/resource/image.hpp>
#include <Match/vulkan/resource/shader_program.hpp>

namespace Match {
    class Renderer;

    class Attachment {
        no_copy_construction(Attachment)
    public:
        Attachment();
        Attachment(const VkAttachmentDescription& description, VkImageUsageFlags usage, VkImageAspectFlags aspect);
        Attachment(Attachment &&rhs);
        void operator=(Attachment &&rhs);
        ~Attachment();
    INNER_VISIBLE:
        std::unique_ptr<Image> image;
        VkImageView image_view;
    };

    class FrameBuffer {
        no_copy_move_construction(FrameBuffer)
    public:
        FrameBuffer(const Renderer &renderer, const std::vector<VkImageView> &image_views);
        ~FrameBuffer();
    INNER_VISIBLE:
        VkFramebuffer framebuffer;
    };

    class FrameBufferSet {
        no_copy_move_construction(FrameBufferSet)
    public:
        FrameBufferSet(const Renderer &renderer);
        ~FrameBufferSet();
    INNER_VISIBLE:
        std::vector<Attachment> attachments;
        std::vector<std::unique_ptr<FrameBuffer>> framebuffers;
        std::vector<std::weak_ptr<ShaderProgram>> shader_programs;
    };
}
