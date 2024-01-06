#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class StorageBuffer {
        default_no_copy_move_construction(StorageBuffer)
    public:
        virtual ~StorageBuffer() = default;
        virtual vk::Buffer get_buffer(uint32_t in_flight_num) = 0;
        virtual uint64_t get_size() = 0;
    };
}
