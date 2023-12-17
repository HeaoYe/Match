#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class FrameBuffer {
        no_copy_move_construction(FrameBuffer)
    public:
        FrameBuffer(uint32_t index);
        ~FrameBuffer();
    INNER_VISIBLE:
        VkFramebuffer framebuffer;
    };

    class FrameBufferSet {
        no_copy_move_construction(FrameBufferSet)
    public:
        FrameBufferSet();
        ~FrameBufferSet();
    INNER_VISIBLE:
        std::vector<std::unique_ptr<FrameBuffer>> framebuffers;
    };
}
