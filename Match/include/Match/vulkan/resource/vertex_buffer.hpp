#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class VertexBuffer {
        no_copy_move_construction(VertexBuffer)
    public:
        VertexBuffer(uint32_t vertex_size, uint32_t count);
        void *map();
        void flush();
        void unmap();
        ~VertexBuffer();
    INNER_VISIBLE:
        uint32_t size;
        bool mapped;
        void *data_ptr;
        VkBuffer staging;
        VmaAllocation staging_allocation;
        VkBuffer buffer;
        VmaAllocation buffer_allocation;
    };
}
