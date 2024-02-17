#pragma once

#include <Match/vulkan/commons.hpp>
#include <Match/vulkan/descriptor_resource/storage_buffer.hpp>

namespace Match {
    class Buffer : public StorageBuffer {
        no_copy_construction(Buffer);
    public:
        Buffer(uint64_t size, vk::BufferUsageFlags buffer_usage, VmaMemoryUsage vma_usage, VmaAllocationCreateFlags vma_flags);
        Buffer(Buffer &&rhs);
        void *map();
        bool is_mapped();
        void unmap();
        ~Buffer();
        vk::Buffer get_buffer(uint32_t in_flight_num) override { return buffer; }
        uint64_t get_size() override { return size; }
    INNER_VISIBLE:
        uint64_t size;
        bool mapped;
        void *data_ptr;
        vk::Buffer buffer;
        VmaAllocation buffer_allocation;
    };

    class InFlightBuffer : public StorageBuffer {
        no_copy_construction(InFlightBuffer);
    public:
        InFlightBuffer(uint64_t size, vk::BufferUsageFlags buffer_usage, VmaMemoryUsage vma_usage, VmaAllocationCreateFlags vma_flags);
        void *map();
        void unmap();
        ~InFlightBuffer();
        vk::Buffer get_buffer(uint32_t in_flight_num) override { return in_flight_buffers[in_flight_num]->get_buffer(0); }
        uint64_t get_size() override { return in_flight_buffers.front()->get_size(); }
    INNER_VISIBLE:
        std::vector<std::unique_ptr<Buffer>> in_flight_buffers;
    };

    class TwoStageBuffer : public StorageBuffer {
        no_copy_move_construction(TwoStageBuffer)
    public:
        TwoStageBuffer(uint64_t size, vk::BufferUsageFlags usage, vk::BufferUsageFlags additional_usage = {});
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
        vk::Buffer get_buffer(uint32_t in_flight_num) override { return buffer->get_buffer(in_flight_num); }
        uint64_t get_size() override { return buffer->get_size(); }
    INNER_VISIBLE:
        std::unique_ptr<Buffer> staging;
        std::unique_ptr<Buffer> buffer;
    };

    class VertexBuffer : public TwoStageBuffer {
        no_copy_move_construction(VertexBuffer)
    public:
        VertexBuffer(uint32_t vertex_size, uint32_t count, vk::BufferUsageFlags additional_usage);
    };
    
    class IndexBuffer : public TwoStageBuffer {
        no_copy_move_construction(IndexBuffer)
    public:
        IndexBuffer(IndexType type, uint32_t count, vk::BufferUsageFlags additional_usage);
    INNER_VISIBLE:
        vk::IndexType type;
    };
}
