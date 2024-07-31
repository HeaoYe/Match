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
#include <Match/vulkan/resource/volume_data.hpp>

namespace Match {
    class ResourceFactory {
        no_copy_move_construction(ResourceFactory)
    public:
        MATCH_API ResourceFactory(const std::string &root);
        MATCH_API std::shared_ptr<RenderPassBuilder> create_render_pass_builder();
        MATCH_API std::shared_ptr<Renderer> create_renderer(std::shared_ptr<RenderPassBuilder> builder);
        MATCH_API std::shared_ptr<Shader> load_shader(const std::string &filename);
        MATCH_API std::shared_ptr<Shader> compile_shader(const std::string &filename, ShaderStage stage);
        MATCH_API std::shared_ptr<Shader> compile_shader_from_string(const std::string &code, ShaderStage stage);
        MATCH_API std::shared_ptr<VertexAttributeSet> create_vertex_attribute_set(const std::vector<InputBindingInfo> &binding_infos);
        MATCH_API std::shared_ptr<GraphicsShaderProgram> create_shader_program(std::weak_ptr<Renderer> renderer, const std::string &subpass_name);
        MATCH_API std::shared_ptr<VertexBuffer> create_vertex_buffer(uint32_t vertex_size, uint32_t count, vk::BufferUsageFlags additional_usage = vk::BufferUsageFlags {});
        MATCH_API std::shared_ptr<IndexBuffer> create_index_buffer(IndexType type, uint32_t count, vk::BufferUsageFlags additional_usage = vk::BufferUsageFlags {});
        MATCH_API std::shared_ptr<DescriptorSet> create_descriptor_set(std::optional<std::weak_ptr<Renderer>> renderer = {});
        MATCH_API std::shared_ptr<PushConstants> create_push_constants(ShaderStages stages, const std::vector<PushConstantInfo> &infos);
        MATCH_API std::shared_ptr<UniformBuffer> create_uniform_buffer(uint64_t size, bool create_for_each_frame_in_flight = false);
        MATCH_API std::shared_ptr<TwoStageBuffer> create_storage_buffer(uint64_t size);
        MATCH_API std::shared_ptr<StorageImage> create_storage_image(uint32_t width, uint32_t height, vk::Format format = vk::Format::eR8G8B8A8Snorm, bool sampled = true, bool enable_clear = false);
        MATCH_API std::shared_ptr<Sampler> create_sampler(const SamplerOptions &options = {});
        MATCH_API std::shared_ptr<Texture> load_texture(const std::string &filename, uint32_t mip_levels = 0);
        MATCH_API std::shared_ptr<Texture> create_texture(const uint8_t *data, uint32_t width, uint32_t height, uint32_t mip_levels = 0);
        MATCH_API std::shared_ptr<Model> load_model(const std::string &filename, const std::vector<std::string> &backlist = {});
        MATCH_API std::shared_ptr<SphereCollect> create_sphere_collect();
        MATCH_API std::shared_ptr<GLTFScene> load_gltf_scene(const std::string &filename, const std::vector<std::string> &load_attributes = {});
        MATCH_API std::shared_ptr<AccelerationStructureBuilder> create_acceleration_structure_builder();
        MATCH_API std::shared_ptr<RayTracingInstanceCollect> create_ray_tracing_instance_collect(bool allow_update = true);
        MATCH_API std::shared_ptr<RayTracingShaderProgram> create_ray_tracing_shader_program();
        MATCH_API std::shared_ptr<ComputeShaderProgram> create_compute_shader_program();
        MATCH_API std::shared_ptr<VolumeData> load_volume_data(const std::string &filename);
        MATCH_API std::shared_ptr<VolumeData> create_volume_data(const std::vector<float> &raw_data);
    INNER_VISIBLE:
        std::string root;
    };
}
