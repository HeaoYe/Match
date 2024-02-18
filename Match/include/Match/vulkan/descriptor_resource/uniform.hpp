#pragma once

#include <Match/vulkan/resource/buffer.hpp>
#include <Match/vulkan/descriptor_resource/storage_buffer.hpp>

namespace Match {
    class MATCH_API UniformBuffer : public StorageBuffer {
        no_copy_move_construction(UniformBuffer);
    public:
        UniformBuffer(uint64_t size, bool create_for_each_frame_in_flight);
        void *get_uniform_ptr();
        ~UniformBuffer();
        vk::Buffer get_buffer(uint32_t in_flight_num) override { return get_match_buffer(in_flight_num).get_buffer(in_flight_num); }
        uint64_t get_size() override { return size; }
    INNER_VISIBLE:
        Buffer &get_match_buffer(uint32_t in_flight_num);
        uint64_t size;
        std::vector<Buffer> buffers;
    };
}
