#pragma once

#include <Match/vulkan/resource/shader.hpp>
#include <Match/vulkan/resource/vertex_attribute_set.hpp>
#include <Match/vulkan/resource/sampler.hpp>
#include <Match/vulkan/descriptor/uniform.hpp>
#include <Match/vulkan/descriptor/texture.hpp>

namespace Match {
    enum class Topology {
        ePointList,
        eLineList,
        eLineStrip,
        eTriangleList,
        eTriangleFan,
    };
    
    enum class PolygonMode {
        eFill,
        eLine,
        ePoint,
    };

    enum class CullMode {
        eNone,
        eFront,
        eBack,
        eFrontAndBack,
    };

    enum class FrontFace {
        eClockwise,
        eCounterClockwise,
    };

    struct ShaderProgramCompileOptions {
        Topology topology = Topology::eTriangleList;
        PolygonMode polygon_mode = PolygonMode::eFill;
        CullMode cull_mode = CullMode::eBack;
        FrontFace front_face = FrontFace::eCounterClockwise;
        std::vector<VkDynamicState> dynamic_states;
    };

    class Renderer;

    class ShaderProgram {
        no_copy_move_construction(ShaderProgram)
        using binding = uint32_t;
    public:
        ShaderProgram(std::weak_ptr<Renderer> renderer, const std::string &subpass_name);
        void bind_vertex_attribute_set(std::shared_ptr<VertexAttributeSet> attribute_set);
        void attach_vertex_shader(std::shared_ptr<Shader> shader, const std::string &entry);
        void attach_fragment_shader(std::shared_ptr<Shader> shader, const std::string &entry);
        void compile(const ShaderProgramCompileOptions &options);
        void bind_uniforms(binding binding, const std::vector<std::shared_ptr<UniformBuffer>> &uniform_buffers);
        void bind_textures(binding binding, const std::vector<std::shared_ptr<Texture>> &textures, const std::vector<std::shared_ptr<Sampler>> &samplers);
        ~ShaderProgram();
    private:
        void bind_shader_descriptor(const std::vector<DescriptorInfo> &descriptor_infos, VkShaderStageFlags stage);
    INNER_VISIBLE:
        std::weak_ptr<Renderer> renderer;
        std::string subpass_name;
        VkPipelineBindPoint bind_point;
        std::shared_ptr<VertexAttributeSet> vertex_attribute_set;
        std::vector<VkDescriptorSet> descriptor_sets;
        std::shared_ptr<Shader> vertex_shader;
        std::string vertex_shader_entry;
        std::shared_ptr<Shader> fragment_shader;
        std::string fragment_shader_entry;
        VkDescriptorSetLayout descriptor_set_layout;
        VkPipelineLayout layout;
        VkPipeline pipeline;
    };
}
