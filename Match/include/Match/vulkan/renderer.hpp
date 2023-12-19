#pragma once

#include <Match/vulkan/command_pool.hpp>
#include <Match/vulkan/resource/shader_program.hpp>
#include <Match/vulkan/resource/vertex_buffer.hpp>

namespace Match {
    class Renderer {
        no_copy_move_construction(Renderer)
    public:
        Renderer(uint32_t max_in_flight_num);
        ~Renderer();
        void begin_render();
        void end_render();
        void bind_shader_program(std::shared_ptr<ShaderProgram> shader_program);
        void bind_vertex_buffers(const std::vector<std::shared_ptr<VertexBuffer>> &vertex_buffers);
        void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
        VkCommandBuffer get_command_buffer();
    INNER_VISIBLE:
        uint32_t index;
        uint32_t current_in_flight, max_in_flight_num;
        std::vector<VkCommandBuffer> command_buffers;
        std::vector<VkSemaphore> image_available_semaphores;
        std::vector<VkSemaphore> render_finished_semaphores;
        std::vector<VkFence> in_flight_fences;
    };
}
