#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class Buffer {
        no_copy_construction(Buffer);
    public:
        Buffer(uint32_t size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage vma_usage, VmaAllocationCreateFlags vma_flags);
        Buffer(Buffer &&rhs);
        void *map();
        void unmap();
        ~Buffer();
    INNER_VISIBLE:
        uint32_t size;
        bool mapped;
        void *data_ptr;
        VkBuffer buffer;
        VmaAllocation buffer_allocation;
    };

    class TwoStageBuffer {
        no_copy_move_construction(TwoStageBuffer)
    public:
        TwoStageBuffer(uint32_t size, VkBufferUsageFlags usage);
        void *map();
        void flush();
        void unmap();
        ~TwoStageBuffer();
    INNER_VISIBLE:
        std::unique_ptr<Buffer> staging;
        std::unique_ptr<Buffer> buffer;
    };

    class VertexBuffer : public TwoStageBuffer {
        no_copy_move_construction(VertexBuffer)
    public:
        VertexBuffer(uint32_t vertex_size, uint32_t count);
    };
    
    class IndexBuffer : public TwoStageBuffer {
        no_copy_move_construction(IndexBuffer)
    public:
        IndexBuffer(IndexType type, uint32_t count);
    INNER_VISIBLE:
        VkIndexType type;
    };
}
