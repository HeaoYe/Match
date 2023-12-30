#pragma once

#include <Match/vulkan/resource/shader.hpp>
#include <Match/vulkan/resource/vertex_attribute_set.hpp>
#include <Match/vulkan/resource/sampler.hpp>
#include <Match/vulkan/descriptor_resource/uniform.hpp>
#include <Match/vulkan/descriptor_resource/texture.hpp>

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
        FrontFace front_face = FrontFace::eClockwise;
        VkBool32 depth_test_enable = VK_FALSE;
        VkBool32 depth_write_enable = VK_TRUE;
        VkCompareOp depth_compere_op = VK_COMPARE_OP_LESS;
        std::vector<VkDynamicState> dynamic_states;
    };

    union BasicConstantValue {
        bool b;
        BasicConstantValue(bool b) : b(b) {}
        int32_t i;
        BasicConstantValue(int32_t i) : i(i) {}
        uint32_t ui;
        BasicConstantValue(uint32_t ui) : ui(ui) {}
        float f;
        BasicConstantValue(float f) : f(f) {}
    };

    class Renderer;

    class ShaderProgram {
        no_copy_move_construction(ShaderProgram)
        using binding = uint32_t;
    public:
        ShaderProgram(std::weak_ptr<Renderer> renderer, const std::string &subpass_name);
        void bind_vertex_attribute_set(std::shared_ptr<VertexAttributeSet> attribute_set);
        void attach_vertex_shader(std::shared_ptr<Shader> shader, const std::string &entry = "main");
        void attach_fragment_shader(std::shared_ptr<Shader> shader, const std::string &entry = "main");
        void compile(const ShaderProgramCompileOptions &options = {});
        void bind_uniforms(binding binding, const std::vector<std::shared_ptr<UniformBuffer>> &uniform_buffers);
        void bind_textures(binding binding, const std::vector<std::shared_ptr<Texture>> &textures, const std::vector<std::shared_ptr<Sampler>> &samplers);
        void bind_input_attachments(binding binding, const std::vector<std::string> &attachment_names, const std::vector<std::shared_ptr<Sampler>> &samplers);
        void push_constants(const std::string &name, BasicConstantValue basic_value);
        void push_constants(const std::string &name, void *data);
        ~ShaderProgram();
    INNER_VISIBLE:
        void update_input_attachments();
    private:
        void bind_shader_descriptor(const std::vector<DescriptorInfo> &descriptor_infos, VkShaderStageFlags stage);
        std::pair<uint32_t, uint32_t> find_offset_size_by_name(Shader &shader, VkShaderStageFlags stage, const std::string &name);
    INNER_VISIBLE:
        std::weak_ptr<Renderer> renderer;
        std::string subpass_name;
        VkPipelineBindPoint bind_point;
        std::shared_ptr<VertexAttributeSet> vertex_attribute_set;
        std::map<uint32_t, std::pair<std::vector<std::string>, std::vector<std::shared_ptr<Sampler>>>> input_attachments_temp;
        std::optional<uint32_t> callback_id;
        std::vector<VkDescriptorSet> descriptor_sets;
        std::map<VkShaderStageFlags, std::pair<uint32_t, uint32_t>> constant_offset_size_map;
        std::vector<uint8_t> constants;
        std::shared_ptr<Shader> vertex_shader;
        std::string vertex_shader_entry;
        std::shared_ptr<Shader> fragment_shader;
        std::string fragment_shader_entry;
        VkDescriptorSetLayout descriptor_set_layout;
        VkPipelineLayout layout;
        VkPipeline pipeline;
    };
}
