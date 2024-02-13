#pragma once

#include <Match/vulkan/command_pool.hpp>
#include <Match/vulkan/renderpass.hpp>
#include <Match/vulkan/framebuffer.hpp>
#include <Match/vulkan/resource/shader_program.hpp>
#include <Match/vulkan/resource/buffer.hpp>
#include <Match/vulkan/resource/model.hpp>

namespace Match {
    class RenderLayer {
        no_copy_move_construction(RenderLayer)
    public:
        RenderLayer(Renderer &renderer) : renderer(renderer) {}
        virtual void begin_render() = 0;
        virtual void end_render() = 0;
        virtual ~RenderLayer() = default;
    protected:
        Renderer &renderer;
    };

    class Renderer {
        no_copy_move_construction(Renderer)
        using ResourceRecreateCallback = std::function<void()>;
    public:
        Renderer(std::shared_ptr<RenderPassBuilder> builder);
        ~Renderer();
        template<class LayerType>
        void attach_render_layer(const std::string &name) {
            layers_map.insert(std::make_pair(name, layers.size()));
            layers.push_back(std::make_unique<LayerType>(*this));
        }
        template<class LayerType, typename... Args>
        void attach_render_layer(const std::string &name, Args &&... args) {
            layers_map.insert(std::make_pair(name, layers.size()));
            layers.push_back(std::make_unique<LayerType>(*this, std::forward<Args>(args)...));
        }
        void acquire_next_image();
        void begin_render_pass();
        void end_render_pass();
        void present();
        void begin_render();
        void end_render();
        void begin_layer_render(const std::string &name);
        void end_layer_render(const std::string &name);
        template <class ShaderProgramClass>
        void bind_shader_program(std::shared_ptr<ShaderProgramClass> shader_program) {
            constexpr auto bind_point = ShaderProgramBindPoint<ShaderProgramClass>::bind_point;
            if constexpr (bind_point == vk::PipelineBindPoint::eGraphics) {
                current_graphics_shader_program = shader_program;
            } else if constexpr (bind_point == vk::PipelineBindPoint::eRayTracingKHR) {
                current_ray_tracing_shader_program = shader_program;
            } else if constexpr (bind_point == vk::PipelineBindPoint::eCompute) {
                current_compute_shader_program = shader_program;
            } else {
                throw std::runtime_error("Match Core Fatal");
            }
            inner_bind_shader_program(bind_point, shader_program);
        }
        void bind_vertex_buffer(const std::shared_ptr<VertexBuffer> &vertex_buffer, uint32_t binding = 0);
        void bind_vertex_buffers(const std::vector<std::shared_ptr<VertexBuffer>> &vertex_buffers, uint32_t first_binding = 0);
        void bind_index_buffer(std::shared_ptr<IndexBuffer> index_buffer);
        void set_viewport(float x, float y, float width, float height);
        void set_viewport(float x, float y, float width, float height, float min_depth, float max_depth);
        void set_scissor(int x, int y, uint32_t width, uint32_t height);
        void next_subpass();
        void continue_subpass_to(const std::string &subpass_name);
        void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
        void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance);
        void draw_mesh(std::shared_ptr<const Mesh> mesh, uint32_t instance_count, uint32_t first_instance);
        void draw_model_mesh(std::shared_ptr<const Model> model, const std::string &name, uint32_t instance_count, uint32_t first_instance);
        void draw_model(std::shared_ptr<const Model> model, uint32_t instance_count, uint32_t first_instance);
        void trace_rays(uint32_t width = uint32_t(-1), uint32_t height = uint32_t(-1), uint32_t depth = 1);
        void dispatch(uint32_t group_count_x, uint32_t group_count_y = 1, uint32_t group_count_z = 1);
        uint32_t register_resource_recreate_callback(const ResourceRecreateCallback &callback);
        void remove_resource_recreate_callback(uint32_t id);
    private:
        void inner_bind_shader_program(vk::PipelineBindPoint bind_point, std::shared_ptr<ShaderProgram> shader_program);
    public:
        void set_clear_value(const std::string &name, const vk::ClearValue &value);
        vk::CommandBuffer get_command_buffer();
        void set_resize_flag();
        void wait_for_destroy();
        void update_resources();
    INNER_VISIBLE:
        void update_renderpass();
    INNER_VISIBLE:
        std::shared_ptr<RenderPassBuilder> render_pass_builder;
        std::unique_ptr<RenderPass> render_pass;
        std::unique_ptr<FrameBufferSet> framebuffer_set;
        std::vector<std::unique_ptr<RenderLayer>> layers;
        std::map<std::string, uint32_t> layers_map;
        uint32_t current_callback_id;
        std::map<uint32_t, ResourceRecreateCallback> callbacks;
        bool resized;
        uint32_t index;
        uint32_t current_in_flight;
        uint32_t current_subpass;
        vk::CommandBuffer current_buffer;
        std::vector<vk::CommandBuffer> command_buffers;
        std::vector<vk::Semaphore> image_available_semaphores;
        std::vector<vk::Semaphore> render_finished_semaphores;
        std::vector<vk::Fence> in_flight_fences;
    private:
        std::shared_ptr<GraphicsShaderProgram> current_graphics_shader_program;
        std::shared_ptr<RayTracingShaderProgram> current_ray_tracing_shader_program;
        std::shared_ptr<ComputeShaderProgram> current_compute_shader_program;
    };
}
