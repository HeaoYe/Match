#pragma once

#include <Match/vulkan/command_pool.hpp>
#include <Match/vulkan/resource/shader_program.hpp>

namespace Match {
    class Renderer {
        no_copy_move_construction(Renderer)
    public:
        Renderer(CommandPool &command_pool, uint32_t max_in_flight_num);
        ~Renderer();
        void begin_render();
        void end_render();
        void bind_shader_program(std::shared_ptr<ShaderProgram> shader_program);
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
