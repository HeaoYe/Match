#pragma once

#include <Match/vulkan/resource/buffer.hpp>

namespace Match {
    class UniformBuffer {
        no_copy_move_construction(UniformBuffer);
    public:
        UniformBuffer(uint32_t size);
        void *get_uniform_ptr();
        ~UniformBuffer();
    INNER_VISIBLE:
        uint32_t size;
        std::vector<Buffer> buffers;
    };
}
