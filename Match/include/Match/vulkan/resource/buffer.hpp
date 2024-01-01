#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class Buffer {
        no_copy_construction(Buffer);
    public:
        Buffer(uint32_t size, vk::BufferUsageFlags buffer_usage, VmaMemoryUsage vma_usage, VmaAllocationCreateFlags vma_flags);
        Buffer(Buffer &&rhs);
        void *map();
        bool is_mapped();
        void unmap();
        ~Buffer();
    INNER_VISIBLE:
        uint32_t size;
        bool mapped;
        void *data_ptr;
        vk::Buffer buffer;
        VmaAllocation buffer_allocation;
    };

    class TwoStageBuffer {
        no_copy_move_construction(TwoStageBuffer)
    public:
        TwoStageBuffer(uint32_t size, vk::BufferUsageFlags usage);
        void *map();
        void flush();
        void unmap();
        template <class Type>
        uint32_t upload_data_from_vector(const std::vector<Type> &data, uint32_t offset_count = 0) {
            auto mapped = staging->is_mapped();
            auto *ptr = static_cast<uint8_t *>(staging->map());
            memcpy(ptr + (offset_count * sizeof(Type)), data.data(), data.size() * sizeof(Type));
            flush();
            if (!mapped) {
                staging->unmap();
            }
            return offset_count + data.size();
        }
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
        vk::IndexType type;
    };
}
