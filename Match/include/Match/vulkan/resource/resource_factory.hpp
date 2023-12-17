#pragma once

#include <Match/commons.hpp>
#include <Match/vulkan/resource/shader.hpp>
#include <Match/vulkan/resource/shader_program.hpp>

namespace Match {
    class ResourceFactory {
        no_copy_move_construction(ResourceFactory)
    public:
        ResourceFactory(const std::string &root);
        std::shared_ptr<Shader> load_shader(const std::string &filename);
        std::shared_ptr<ShaderProgram> create_shader_program(const std::string &subpass_name);
    INNER_VISIBLE:
        std::string root;
    };
}
