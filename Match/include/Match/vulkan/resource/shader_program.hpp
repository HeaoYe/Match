#pragma once

#include <Match/vulkan/resource/shader.hpp>
#include <Match/vulkan/resource/vertex_attribute_set.hpp>
#include <Match/vulkan/resource/push_constants.hpp>
#include <Match/vulkan/descriptor_resource/descriptor_set.hpp>

namespace Match {
    struct GraphicsShaderProgramCompileOptions {
        Topology topology = Topology::eTriangleList;
        PolygonMode polygon_mode = PolygonMode::eFill;
        CullMode cull_mode = CullMode::eBack;
        FrontFace front_face = FrontFace::eClockwise;
        vk::Bool32 depth_test_enable = VK_FALSE;
        vk::Bool32 depth_write_enable = VK_TRUE;
        vk::CompareOp depth_compere_op = vk::CompareOp::eLess;
        std::vector<vk::DynamicState> dynamic_states;
    };

    struct RayTracingShaderProgramCompileOptions {
        uint32_t max_ray_recursion_depth = 1;
    };

    struct ComputeShaderProgramCompileOptions {};

    class Renderer;

    template <class ShaderProgramClass>
    struct ShaderProgramBindPoint {
        const static vk::PipelineBindPoint bind_point;
    };

    class ShaderProgram {
        default_no_copy_move_construction(ShaderProgram)
    public:
        virtual ~ShaderProgram();
    protected:
        void compile_pipeline_layout();
    INNER_PROTECT:
        std::vector<std::optional<std::shared_ptr<DescriptorSet>>> descriptor_sets;
        std::optional<std::shared_ptr<PushConstants>> push_constants;
        vk::PipelineLayout layout;
        vk::Pipeline pipeline;
    };

    template <class SubClass, typename OptionType>
    class ShaderProgramTemplate : public ShaderProgram {
        default_no_copy_move_construction(ShaderProgramTemplate)
    INNER_PROTECT:
        struct ShaderStageInfo {
            ShaderStageInfo() : shader(nullptr), entry() {}
            ShaderStageInfo(std::shared_ptr<Shader> shader) : shader(shader), entry("main") {}
            ShaderStageInfo(std::shared_ptr<Shader> shader, const std::string &entry) : shader(shader), entry(entry) {}
            std::shared_ptr<Shader> shader;
            std::string entry;
        };
    public:
        SubClass &attach_descriptor_set(std::shared_ptr<DescriptorSet> descriptor_set, uint32_t set_index = 0) {
            if (set_index + 1 > descriptor_sets.size()) {
                descriptor_sets.resize(set_index + 1);
            }
            descriptor_sets[set_index] = descriptor_set;
            return *dynamic_cast<SubClass *>(this);
        }

        SubClass &attach_push_constants(std::shared_ptr<PushConstants> push_constants) {
            if (this->push_constants.has_value()) {
                this->push_constants.value().reset();
                this->push_constants->reset();
            }
            this->push_constants = push_constants;
            return *dynamic_cast<SubClass *>(this);
        }
        
        virtual ~ShaderProgramTemplate() override = default;
        virtual SubClass &compile(const OptionType &options = {}) = 0;
    };

    class GraphicsShaderProgram : public ShaderProgramTemplate<GraphicsShaderProgram, GraphicsShaderProgramCompileOptions> {
        no_copy_move_construction(GraphicsShaderProgram)
    public:
        GraphicsShaderProgram(std::weak_ptr<Renderer> renderer, const std::string &subpass_name);
        GraphicsShaderProgram &attach_vertex_attribute_set(std::shared_ptr<VertexAttributeSet> attribute_set);
        GraphicsShaderProgram &attach_vertex_shader(std::shared_ptr<Shader> shader, const std::string &entry = "main");
        GraphicsShaderProgram &attach_fragment_shader(std::shared_ptr<Shader> shader, const std::string &entry = "main");
        GraphicsShaderProgram &compile(const GraphicsShaderProgramCompileOptions &options = {}) override;
        ~GraphicsShaderProgram() override;
    INNER_VISIBLE:
        void update_input_attachments();
    INNER_VISIBLE:
        std::weak_ptr<Renderer> renderer;
        std::string subpass_name;
        std::shared_ptr<VertexAttributeSet> vertex_attribute_set;
        ShaderStageInfo vertex_shader {};
        ShaderStageInfo fragment_shader {};
    };

    template <>
    struct ShaderProgramBindPoint<GraphicsShaderProgram> {
        const static vk::PipelineBindPoint bind_point = vk::PipelineBindPoint::eGraphics;
    };

    class RayTracingShaderProgram : public ShaderProgramTemplate<RayTracingShaderProgram, RayTracingShaderProgramCompileOptions> {
        no_copy_move_construction(RayTracingShaderProgram)
    INNER_VISIBLE:
        struct HitGroup {
            ShaderStageInfo closest_hit_shader;
            std::optional<ShaderStageInfo> intersection_shader;
        };
    public:
        RayTracingShaderProgram();
        RayTracingShaderProgram &attach_raygen_shader(std::shared_ptr<Shader> shader, const std::string &entry = "main");
        RayTracingShaderProgram &attach_miss_shader(std::shared_ptr<Shader> shader, const std::string &entry = "main");
        RayTracingShaderProgram &attach_hit_group(const ShaderStageInfo &closest_hit_shader, const std::optional<ShaderStageInfo> &intersection_shader = {});
        RayTracingShaderProgram &compile(const RayTracingShaderProgramCompileOptions &options = {}) override;
        ~RayTracingShaderProgram() override;
    INNER_VISIBLE:
        ShaderStageInfo raygen_shader {};
        std::vector<ShaderStageInfo> miss_shaders;
        std::vector<HitGroup> hit_groups;
        uint32_t hit_shader_count;

        std::unique_ptr<Buffer> shader_binding_table_buffer;
        vk::StridedDeviceAddressRegionKHR raygen_region {};
        vk::StridedDeviceAddressRegionKHR miss_region {};
        vk::StridedDeviceAddressRegionKHR hit_region {};
        vk::StridedDeviceAddressRegionKHR callable_region {};
    };

    template <>
    struct ShaderProgramBindPoint<RayTracingShaderProgram> {
        const static vk::PipelineBindPoint bind_point = vk::PipelineBindPoint::eRayTracingKHR;
    };

    class ComputeShaderProgram : public ShaderProgramTemplate<ComputeShaderProgram, ComputeShaderProgramCompileOptions> {
        no_copy_move_construction(ComputeShaderProgram)
    public:
        ComputeShaderProgram();
        ComputeShaderProgram &attach_compute_shader(std::shared_ptr<Shader> shader, const std::string &entry = "main");
        ComputeShaderProgram &compile(const ComputeShaderProgramCompileOptions &options = {}) override;
        ~ComputeShaderProgram() override;
    INNER_VISIBLE:
        ShaderStageInfo compute_shader;
    };

    template <>
    struct ShaderProgramBindPoint<ComputeShaderProgram> {
        const static vk::PipelineBindPoint bind_point = vk::PipelineBindPoint::eCompute;
    };
}
