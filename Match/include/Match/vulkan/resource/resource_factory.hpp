#pragma once

#include <Match/commons.hpp>
#include <Match/vulkan/resource/shader.hpp>
#include <Match/vulkan/resource/shader_program.hpp>
#include <Match/vulkan/resource/vertex_buffer.hpp>

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
        std::shared_ptr<Shader> load_shader(const std::string &filename, ShaderType type = ShaderType::eCompiled);
        std::shared_ptr<VertexAttributeSet> create_vertex_attribute();
        std::shared_ptr<ShaderProgram> create_shader_program(const std::string &subpass_name);
        std::shared_ptr<VertexBuffer> create_vertex_buffer(uint32_t vertex_size, uint32_t count);
    INNER_VISIBLE:
        std::string root;
    };
}
