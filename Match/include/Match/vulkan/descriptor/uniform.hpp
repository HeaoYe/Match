#pragma once

#include <Match/vulkan/resource/buffer.hpp>
#include <Match/vulkan/descriptor/descriptor.hpp>

namespace Match {
    class UniformBuffer : Descriptor {
        no_copy_move_construction(UniformBuffer);
    public:
        UniformBuffer(uint32_t size);
        void *get_uniform_ptr();
        ~UniformBuffer();
    INNER_VISIBLE:
        std::vector<Buffer> buffers;
    };
}
