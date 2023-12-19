#pragma once

#include <Match/vulkan/resource/shader.hpp>

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

    enum class InputRate {
        ePerVertex,
        ePerInstance,
    };

    enum class VertexType {
        eInt8, eInt8x2, eInt8x3, eInt8x4,
        eInt16, eInt16x2, eInt16x3, eInt16x4,
        eInt32, eInt32x2, eInt32x3, eInt32x4,
        eInt64, eInt64x2, eInt64x3, eInt64x4,
        eUint8, eUint8x2, eUint8x3, eUint8x4,
        eUint16, eUint16x2, eUint16x3, eUint16x4,
        eUint32, eUint32x2, eUint32x3, eUint32x4,
        eUint64, eUint64x2, eUint64x3, eUint64x4,
        eFloat, eFloat2, eFloat3, eFloat4,
        eDouble, eDouble2, eDouble3, eDouble4,
    };

    struct ShaderProgramCompileOptions {
        Topology topology = Topology::eTriangleList;
        PolygonMode polygon_mode = PolygonMode::eFill;
        CullMode cull_mode = CullMode::eBack;
        FrontFace front_face = FrontFace::eCounterClockwise;
    };

    class  VertexAttributeSet {
        no_copy_move_construction(VertexAttributeSet)
        using location = uint32_t;
        using binding = uint32_t;
    public:
        VertexAttributeSet();
        location add_input_attribute(VertexType type);
        binding add_input_binding(InputRate rate);
    private:
        void reset();
    INNER_VISIBLE:
        std::vector<VkVertexInputAttributeDescription> attributes;
        std::vector<VkVertexInputBindingDescription> bindings;
    private:
        uint32_t current_binding;
        uint32_t current_location;
        uint32_t current_offset;
    };

    class ShaderProgram {
        no_copy_move_construction(ShaderProgram)
    public:
        ShaderProgram(const std::string &subpass_name);
        void bind_vertex_attribute_set(std::shared_ptr<VertexAttributeSet> attribute);
        void attach_vertex_shader(std::shared_ptr<Shader> shader, const std::string &entry);
        void attach_fragment_shader(std::shared_ptr<Shader> shader, const std::string &entry);
        void compile(ShaderProgramCompileOptions options);
        ~ShaderProgram();
    INNER_VISIBLE:
        std::string subpass_name;
        VkPipelineBindPoint bind_point;
        std::shared_ptr<VertexAttributeSet> vertex_attribute;
        std::shared_ptr<Shader> vertex_shader;
        std::string vertex_shader_entry;
        std::shared_ptr<Shader> fragment_shader;
        std::string fragment_shader_entry;
        VkPipelineLayout layout;
        VkPipeline pipeline;
    };
}
