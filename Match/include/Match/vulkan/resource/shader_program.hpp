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

    struct ShaderProgramCompileOptions {
        Topology topology = Topology::eTriangleList;
        PolygonMode polygon_mode = PolygonMode::eFill;
        CullMode cull_mode = CullMode::eBack;
        FrontFace front_face = FrontFace::eCounterClockwise;
    };

    class ShaderProgram {
        no_copy_move_construction(ShaderProgram)
    public:
        ShaderProgram(const std::string &subpass_name);
        void attach_vertex_shader(std::shared_ptr<Shader> shader, const std::string &entry);
        void attach_fragment_shader(std::shared_ptr<Shader> shader, const std::string &entry);
        void compile(ShaderProgramCompileOptions options);
        ~ShaderProgram();
    INNER_VISIBLE:
        std::string subpass_name;
        VkPipelineBindPoint bind_point;
        std::shared_ptr<Shader> vertex_shader;
        std::string vertex_shader_entry;
        std::shared_ptr<Shader> fragment_shader;
        std::string fragment_shader_entry;
        VkPipelineLayout layout;
        VkPipeline pipeline;
    };
}
