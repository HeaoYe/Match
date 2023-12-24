#include <Match/vulkan/resource/buffer.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"

namespace Match {
    Buffer::Buffer(uint32_t size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage vma_usage, VmaAllocationCreateFlags vma_flags) : size(size), mapped(false), data_ptr(nullptr) {
        VkBufferCreateInfo buffer_create_info { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        buffer_create_info.size = size;
        buffer_create_info.usage = buffer_usage;
        VmaAllocationCreateInfo buffer_alloc_info {};
        buffer_alloc_info.flags = vma_flags;
        buffer_alloc_info.usage = vma_usage;
        vmaCreateBuffer(manager->vma_allocator, &buffer_create_info, &buffer_alloc_info, &buffer, &buffer_allocation, nullptr);
    }

    Buffer::Buffer(Buffer &&rhs) {
        size = rhs.size;
        mapped = rhs.mapped;
        data_ptr = rhs.data_ptr;
        buffer = rhs.buffer;
        buffer_allocation = rhs.buffer_allocation;
        rhs.size = 0;
        rhs.mapped = false;
        rhs.data_ptr = nullptr;
        rhs.buffer = VK_NULL_HANDLE;
        rhs.buffer_allocation = NULL;
    }

    void *Buffer::map() {
        if (mapped) {
            return data_ptr;
        }
        mapped = true;
        vmaMapMemory(manager->vma_allocator, buffer_allocation, &data_ptr);
        return data_ptr;
    }

    bool Buffer::is_mapped() {
        return mapped;
    }

    void Buffer::unmap() {
        if (!mapped) {
            return;
        }
        mapped = false;
        vmaUnmapMemory(manager->vma_allocator, buffer_allocation);
    }

    Buffer::~Buffer() {
        unmap();
        vmaDestroyBuffer(manager->vma_allocator, buffer, buffer_allocation);
    }

    TwoStageBuffer::TwoStageBuffer(uint32_t size, VkBufferUsageFlags usage) {
        staging = std::make_unique<Buffer>(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
        buffer = std::make_unique<Buffer>(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);        
    }

    void *TwoStageBuffer::map() {
        return staging->map();
    }
    
    void TwoStageBuffer::flush() {
        auto command_buffer = manager->command_pool->allocate_single_use();
        VkBufferCopy copy;
        copy.srcOffset = 0;
        copy.dstOffset = 0;
        copy.size = staging->size;
        vkCmdCopyBuffer(command_buffer, staging->buffer, buffer->buffer, 1, &copy);
        manager->command_pool->free_single_use(command_buffer);
    }

    void TwoStageBuffer::unmap() {
        staging->unmap();
    }

    TwoStageBuffer::~TwoStageBuffer() {
        staging.reset();
        buffer.reset();
    }

    VertexBuffer::VertexBuffer(uint32_t vertex_size, uint32_t count) : TwoStageBuffer(vertex_size * count, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {}
    IndexBuffer::IndexBuffer(IndexType type, uint32_t count) : type(transform<VkIndexType>(type)), TwoStageBuffer(transform<uint32_t>(type) * count, VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {}
}
