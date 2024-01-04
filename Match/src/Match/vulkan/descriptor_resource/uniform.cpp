#include <Match/vulkan/descriptor_resource/uniform.hpp>
#include <Match/core/setting.hpp>

namespace Match {
    UniformBuffer::UniformBuffer(uint32_t size, bool create_for_each_frame_in_flight) : size(size) {
        for (uint32_t i = 0; i < (create_for_each_frame_in_flight ? setting.max_in_flight_frame : 1); i ++) {
            buffers.emplace_back(size, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
            buffers.back().map();
        }
    }

    void *UniformBuffer::get_uniform_ptr() {
        return get_buffer(runtime_setting->current_in_flight).data_ptr;
    }

    Buffer &UniformBuffer::get_buffer(uint32_t in_flight_index) {
        return buffers[in_flight_index % buffers.size()];
    }

    UniformBuffer::~UniformBuffer() {
        buffers.clear();
    }
}
