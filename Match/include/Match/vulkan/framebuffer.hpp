#pragma once

#include <Match/vulkan/commons.hpp>
#include <Match/vulkan/resource/image.hpp>
#include <Match/vulkan/resource/shader_program.hpp>

namespace Match {
    class Renderer;

    class Attachment {
        no_copy_construction(Attachment)
    public:
        MATCH_API Attachment();
        MATCH_API Attachment(const vk::AttachmentDescription& description, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect);
        MATCH_API Attachment(Attachment &&rhs);
        MATCH_API void operator=(Attachment &&rhs);
        MATCH_API ~Attachment();
    INNER_VISIBLE:
        std::unique_ptr<Image> image;
        vk::ImageView image_view;
    };

    class FrameBuffer {
        no_copy_move_construction(FrameBuffer)
    public:
        MATCH_API FrameBuffer(const Renderer &renderer, const std::vector<vk::ImageView> &image_views);
        MATCH_API ~FrameBuffer();
    INNER_VISIBLE:
        vk::Framebuffer framebuffer;
    };

    class FrameBufferSet {
        no_copy_move_construction(FrameBufferSet)
    public:
        MATCH_API FrameBufferSet(const Renderer &renderer);
        MATCH_API ~FrameBufferSet();
    INNER_VISIBLE:
        std::vector<Attachment> attachments;
        std::vector<std::unique_ptr<FrameBuffer>> framebuffers;
    };
}
