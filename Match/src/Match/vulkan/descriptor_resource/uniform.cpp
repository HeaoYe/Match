#include <Match/vulkan/descriptor_resource/uniform.hpp>
#include <Match/core/setting.hpp>

namespace Match {
    UniformBuffer::UniformBuffer(uint32_t size) : size(size) {
        for (uint32_t i = 0; i < setting.max_in_flight_frame; i ++) {
            buffers.emplace_back(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
            buffers.back().map();
        }
    }

    void *UniformBuffer::get_uniform_ptr() {
        return buffers[runtime_setting->current_in_flight].data_ptr;
    }

    UniformBuffer::~UniformBuffer() {
        buffers.clear();
    }
}
