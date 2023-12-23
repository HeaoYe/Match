#pragma once

#include <Match/commons.hpp>
#include <Match/vulkan/renderpass.hpp>
#include <Match/vulkan/renderer.hpp>
#include <Match/vulkan/resource/shader.hpp>
#include <Match/vulkan/resource/vertex_attribute_set.hpp>
#include <Match/vulkan/resource/shader_program.hpp>
#include <Match/vulkan/resource/buffer.hpp>
#include <Match/vulkan/resource/sampler.hpp>
#include <Match/vulkan/descriptor_resource/uniform.hpp>
#include <Match/vulkan/descriptor_resource/texture.hpp>

namespace Match {
    enum class ShaderType {
        eCompiled,
        eVertexShaderNeedCompile,
        eFragmentShaderNeedCompile,
    };

    class ResourceFactory {
        no_copy_move_construction(ResourceFactory)
    public:
        ResourceFactory(const std::string &root);
        std::shared_ptr<RenderPassBuilder> create_render_pass_builder();
        std::shared_ptr<Shader> load_shader(const std::string &filename, ShaderType type = ShaderType::eCompiled);
        std::shared_ptr<VertexAttributeSet> create_vertex_attribute_set(const std::vector<InputBindingInfo> &binding_infos);
        std::shared_ptr<ShaderProgram> create_shader_program(std::weak_ptr<Renderer> renderer, const std::string &subpass_name);
        std::shared_ptr<VertexBuffer> create_vertex_buffer(uint32_t vertex_size, uint32_t count);
        std::shared_ptr<IndexBuffer> create_index_buffer(IndexType type, uint32_t count);
        std::shared_ptr<Sampler> create_sampler(const SamplerOptions &options);
        std::shared_ptr<UniformBuffer> create_uniform_buffer(uint32_t size);
        std::shared_ptr<Texture> load_texture(const std::string &filename, uint32_t mip_levels);
    INNER_VISIBLE:
        std::string root;
    };
}
