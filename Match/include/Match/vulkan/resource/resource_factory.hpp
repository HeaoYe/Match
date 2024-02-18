#pragma once

#include <Match/commons.hpp>
#include <Match/vulkan/renderpass.hpp>
#include <Match/vulkan/renderer.hpp>
#include <Match/vulkan/resource/shader.hpp>
#include <Match/vulkan/resource/vertex_attribute_set.hpp>
#include <Match/vulkan/resource/push_constants.hpp>
#include <Match/vulkan/resource/shader_program.hpp>
#include <Match/vulkan/resource/buffer.hpp>
#include <Match/vulkan/resource/sampler.hpp>
#include <Match/vulkan/resource/model.hpp>
#include <Match/vulkan/resource/gltf_scene.hpp>
#include <Match/vulkan/descriptor_resource/uniform.hpp>
#include <Match/vulkan/descriptor_resource/texture.hpp>
#include <Match/vulkan/resource/ray_tracing_instance_collect.hpp>

namespace Match {
    class MATCH_API ResourceFactory {
        no_copy_move_construction(ResourceFactory)
    public:
        ResourceFactory(const std::string &root);
        std::shared_ptr<RenderPassBuilder> create_render_pass_builder();
        std::shared_ptr<Renderer> create_renderer(std::shared_ptr<RenderPassBuilder> builder);
        std::shared_ptr<Shader> load_shader(const std::string &filename);
        std::shared_ptr<Shader> compile_shader(const std::string &filename, ShaderStage stage);
        std::shared_ptr<Shader> compile_shader_from_string(const std::string &code, ShaderStage stage);
        std::shared_ptr<VertexAttributeSet> create_vertex_attribute_set(const std::vector<InputBindingInfo> &binding_infos);
        std::shared_ptr<GraphicsShaderProgram> create_shader_program(std::weak_ptr<Renderer> renderer, const std::string &subpass_name);
        std::shared_ptr<VertexBuffer> create_vertex_buffer(uint32_t vertex_size, uint32_t count, vk::BufferUsageFlags additional_usage = vk::BufferUsageFlags {});
        std::shared_ptr<IndexBuffer> create_index_buffer(IndexType type, uint32_t count, vk::BufferUsageFlags additional_usage = vk::BufferUsageFlags {});
        std::shared_ptr<DescriptorSet> create_descriptor_set(std::optional<std::weak_ptr<Renderer>> renderer = {});
        std::shared_ptr<PushConstants> create_push_constants(ShaderStages stages, const std::vector<PushConstantInfo> &infos);
        std::shared_ptr<UniformBuffer> create_uniform_buffer(uint64_t size, bool create_for_each_frame_in_flight = false);
        std::shared_ptr<StorageImage> create_storage_image(uint32_t width, uint32_t height, vk::Format format = vk::Format::eR8G8B8A8Snorm, bool sampled = true, bool enable_clear = false);
        std::shared_ptr<Sampler> create_sampler(const SamplerOptions &options = {});
        std::shared_ptr<Texture> load_texture(const std::string &filename, uint32_t mip_levels = 0);
        std::shared_ptr<Texture> create_texture(const uint8_t *data, uint32_t width, uint32_t height, uint32_t mip_levels = 0);
        std::shared_ptr<Model> load_model(const std::string &filename);
        std::shared_ptr<SphereCollect> create_sphere_collect();
        std::shared_ptr<GLTFScene> load_gltf_scene(const std::string &filename, const std::vector<std::string> &load_attributes = {});
        std::shared_ptr<AccelerationStructureBuilder> create_acceleration_structure_builder();
        std::shared_ptr<RayTracingInstanceCollect> create_ray_tracing_instance_collect(bool allow_update = true);
        std::shared_ptr<RayTracingShaderProgram> create_ray_tracing_shader_program();
        std::shared_ptr<ComputeShaderProgram> create_compute_shader_program();
    INNER_VISIBLE:
        std::string root;
    };
}
