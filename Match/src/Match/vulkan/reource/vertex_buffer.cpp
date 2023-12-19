#include <Match/vulkan/resource/vertex_buffer.hpp>
#include "../inner.hpp"

namespace Match {
    VertexBuffer::VertexBuffer(uint32_t vertex_size, uint32_t count) : size(vertex_size * count), mapped(false) {
        VkBufferCreateInfo staging_creata_info { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO }, buffer_create_info { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        staging_creata_info.size = size;
        staging_creata_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_create_info.size = size;
        buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        

        VmaAllocationCreateInfo staging_alloc_info {}, buffer_alloc_info {};
        staging_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
        staging_alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        vmaCreateBuffer(manager->vma_allocator, &staging_creata_info, &staging_alloc_info, &staging, &staging_allocation, nullptr);
        buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        buffer_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaCreateBuffer(manager->vma_allocator, &buffer_create_info, &buffer_alloc_info, &buffer, &buffer_allocation, nullptr);
    }

    void *VertexBuffer::map() {
        if (mapped) {
            return data_ptr;
        }
        mapped = true;
        vmaMapMemory(manager->vma_allocator, staging_allocation, &data_ptr);
        return data_ptr;
    }

    void VertexBuffer::flush() {
        auto command_buffer = manager->command_pool->allocate_single_use();
        VkBufferCopy copy;
        copy.srcOffset = 0;
        copy.dstOffset = 0;
        copy.size = size;
        vkCmdCopyBuffer(command_buffer, staging, buffer, 1, &copy);
        manager->command_pool->free_single_use(command_buffer);
    }

    void VertexBuffer::unmap() {
        if (!mapped) {
            return;
        }
        mapped = false;
        flush();
        vmaUnmapMemory(manager->vma_allocator, staging_allocation);
    }

    VertexBuffer::~VertexBuffer() {
        unmap();
        vmaDestroyBuffer(manager->vma_allocator, staging, staging_allocation);
        vmaDestroyBuffer(manager->vma_allocator, buffer, buffer_allocation);
    }
}
