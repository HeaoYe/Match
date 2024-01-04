#pragma once

#include <Match/vulkan/resource/buffer.hpp>

namespace Match {
    class UniformBuffer {
        no_copy_move_construction(UniformBuffer);
    public:
        UniformBuffer(uint32_t size, bool create_for_each_frame_in_flight);
        void *get_uniform_ptr();
        ~UniformBuffer();
    INNER_VISIBLE:
        Buffer &get_buffer(uint32_t in_flight_index);
    INNER_VISIBLE:
        uint32_t size;
        std::vector<Buffer> buffers;
    };
}
