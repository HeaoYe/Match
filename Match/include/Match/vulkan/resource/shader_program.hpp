#pragma once

#include <Match/vulkan/resource/shader.hpp>
#include <Match/vulkan/resource/vertex_attribute_set.hpp>
#include <Match/vulkan/descriptor_resource/descriptor_set.hpp>

namespace Match {
    struct ShaderProgramCompileOptions {
        Topology topology = Topology::eTriangleList;
        PolygonMode polygon_mode = PolygonMode::eFill;
        CullMode cull_mode = CullMode::eBack;
        FrontFace front_face = FrontFace::eClockwise;
        vk::Bool32 depth_test_enable = VK_FALSE;
        vk::Bool32 depth_write_enable = VK_TRUE;
        vk::CompareOp depth_compere_op = vk::CompareOp::eLess;
        std::vector<vk::DynamicState> dynamic_states;
    };

    class Renderer;

    class ShaderProgram {
        no_copy_move_construction(ShaderProgram)
    public:
        ShaderProgram(std::weak_ptr<Renderer> renderer, const std::string &subpass_name);
        ShaderProgram &attach_vertex_attribute_set(std::shared_ptr<VertexAttributeSet> attribute_set);
        ShaderProgram &attach_vertex_shader(std::shared_ptr<Shader> shader, const std::string &entry = "main");
        ShaderProgram &attach_fragment_shader(std::shared_ptr<Shader> shader, const std::string &entry = "main");
        ShaderProgram &attach_descriptor_set(std::shared_ptr<DescriptorSet> descriptor_set, uint32_t set_index = 0);
        ShaderProgram &compile(const ShaderProgramCompileOptions &options = {});
        ShaderProgram &push_constants(const std::string &name, BasicConstantValue basic_value);
        ShaderProgram &push_constants(const std::string &name, void *data);
        ~ShaderProgram();
    INNER_VISIBLE:
        void update_input_attachments();
    private:
        void bind_shader_descriptor(const std::vector<DescriptorInfo> &descriptor_infos, vk::ShaderStageFlags stage);
        std::pair<uint32_t, uint32_t> find_offset_size_by_name(Shader &shader, vk::ShaderStageFlags stage, const std::string &name);
    INNER_VISIBLE:
        std::weak_ptr<Renderer> renderer;
        std::string subpass_name;
        vk::PipelineBindPoint bind_point;
        std::shared_ptr<VertexAttributeSet> vertex_attribute_set;
        std::vector<std::optional<std::shared_ptr<DescriptorSet>>> descriptor_sets;
        std::map<vk::ShaderStageFlags, std::pair<uint32_t, uint32_t>> constant_offset_size_map;
        std::vector<uint8_t> constants;
        std::shared_ptr<Shader> vertex_shader;
        std::string vertex_shader_entry;
        std::shared_ptr<Shader> fragment_shader;
        std::string fragment_shader_entry;
        vk::PipelineLayout layout;
        vk::Pipeline pipeline;
    };
}
