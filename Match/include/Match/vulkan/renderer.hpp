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
        MATCH_API Renderer(std::shared_ptr<RenderPassBuilder> builder);
        MATCH_API ~Renderer();
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
        MATCH_API void acquire_next_image();
        MATCH_API void begin_render_pass();
        MATCH_API void end_render_pass();
        MATCH_API void report_submit_info(const vk::SubmitInfo &submit_info);
        MATCH_API void present(const std::vector<vk::PipelineStageFlags> &wait_stages = {}, const std::vector<vk::Semaphore> &wait_samaphores = {});
        MATCH_API void begin_render();
        MATCH_API void end_render();
        MATCH_API void begin_layer_render(const std::string &name);
        MATCH_API void end_layer_render(const std::string &name);
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
        MATCH_API void bind_vertex_buffer(const std::shared_ptr<VertexBuffer> &vertex_buffer, uint32_t binding = 0);
        MATCH_API void bind_vertex_buffers(const std::vector<std::shared_ptr<VertexBuffer>> &vertex_buffers, uint32_t first_binding = 0);
        MATCH_API void bind_index_buffer(std::shared_ptr<IndexBuffer> index_buffer);
        MATCH_API void set_viewport(float x, float y, float width, float height);
        MATCH_API void set_viewport(float x, float y, float width, float height, float min_depth, float max_depth);
        MATCH_API void set_scissor(int x, int y, uint32_t width, uint32_t height);
        MATCH_API void next_subpass();
        MATCH_API void continue_subpass_to(const std::string &subpass_name);
        MATCH_API void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
        MATCH_API void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance);
        MATCH_API void draw_mesh(std::shared_ptr<const Mesh> mesh, uint32_t instance_count, uint32_t first_instance);
        MATCH_API void draw_model_mesh(std::shared_ptr<const Model> model, const std::string &name, uint32_t instance_count, uint32_t first_instance);
        MATCH_API void draw_model(std::shared_ptr<const Model> model, uint32_t instance_count, uint32_t first_instance);
        MATCH_API void trace_rays(uint32_t width = uint32_t(-1), uint32_t height = uint32_t(-1), uint32_t depth = 1);
        MATCH_API void dispatch(uint32_t group_count_x, uint32_t group_count_y = 1, uint32_t group_count_z = 1);
        MATCH_API uint32_t register_resource_recreate_callback(const ResourceRecreateCallback &callback);
        MATCH_API void remove_resource_recreate_callback(uint32_t id);
    private:
        MATCH_API void inner_bind_shader_program(vk::PipelineBindPoint bind_point, std::shared_ptr<ShaderProgram> shader_program);
    public:
        MATCH_API void set_clear_value(const std::string &name, const vk::ClearValue &value);
        MATCH_API vk::CommandBuffer get_command_buffer();
        MATCH_API void set_resize_flag();
        MATCH_API void wait_for_destroy();
        MATCH_API void update_resources();
        MATCH_API void update_renderpass();
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
        std::vector<std::vector<vk::SubmitInfo>> in_flight_submit_infos;
    private:
        std::shared_ptr<GraphicsShaderProgram> current_graphics_shader_program;
        std::shared_ptr<RayTracingShaderProgram> current_ray_tracing_shader_program;
        std::shared_ptr<ComputeShaderProgram> current_compute_shader_program;
    };
}
