#pragma once

#include <Match/vulkan/resource/shader.hpp>
#include <Match/vulkan/descriptor/descriptor.hpp>
#include <Match/vulkan/resource/vertex_attribute_set.hpp>

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

    class ShaderProgram {
        no_copy_move_construction(ShaderProgram)
        using binding = uint32_t;
    public:
        ShaderProgram(const std::string &subpass_name);
        void bind_vertex_attribute_set(std::shared_ptr<VertexAttributeSet> attribute_set);
        void attach_vertex_shader(std::shared_ptr<Shader> shader, const std::string &entry);
        void attach_fragment_shader(std::shared_ptr<Shader> shader, const std::string &entry);
        void bind_fragment_shader_descriptor(const std::vector<DescriptorInfo> &descriptor_infos);
        void compile(ShaderProgramCompileOptions options);
        std::shared_ptr<Descriptor> get_descriptor(binding binding);
        ~ShaderProgram();
    private:
        void bind_shader_descriptor(const std::vector<DescriptorInfo> &descriptor_infos, VkShaderStageFlags stage);
    INNER_VISIBLE:
        std::string subpass_name;
        VkPipelineBindPoint bind_point;
        std::shared_ptr<VertexAttributeSet> vertex_attribute_set;
        std::map<binding, std::shared_ptr<Descriptor>> descriptors;
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
