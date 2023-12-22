#pragma once

#include <Match/vulkan/command_pool.hpp>
#include <Match/vulkan/renderpass.hpp>
#include <Match/vulkan/framebuffer.hpp>
#include <Match/vulkan/resource/shader_program.hpp>
#include <Match/vulkan/resource/buffer.hpp>

namespace Match {
    class Renderer {
        no_copy_move_construction(Renderer)
    public:
        Renderer(std::shared_ptr<RenderPassBuilder> builder);
        ~Renderer();
        void set_clear_value(const std::string &name, const VkClearValue &value);
        void begin_render();
        void end_render();
        void bind_shader_program(std::shared_ptr<ShaderProgram> shader_program);
        void bind_vertex_buffers(const std::vector<std::shared_ptr<VertexBuffer>> &vertex_buffers);
        void bind_index_buffer(std::shared_ptr<IndexBuffer> index_buffer);
        void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
        void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance);
        VkCommandBuffer get_command_buffer();
        void set_resize_flag();
        void wait_for_destroy();
    private:
        void recreate();
    INNER_VISIBLE:
        std::shared_ptr<RenderPassBuilder> render_pass_builder;
        std::unique_ptr<RenderPass> render_pass;
        std::unique_ptr<FrameBufferSet> framebuffer_set;
        bool resized;
        uint32_t index;
        uint32_t current_in_flight;
        VkCommandBuffer current_buffer;
        std::vector<VkCommandBuffer> command_buffers;
        std::vector<VkSemaphore> image_available_semaphores;
        std::vector<VkSemaphore> render_finished_semaphores;
        std::vector<VkFence> in_flight_fences;
    };
}
