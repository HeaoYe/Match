#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class Shader {
        no_copy_move_construction(Shader)
    public:
        Shader(const std::string &filename);
        ~Shader();
    INNER_VISIBLE:
        VkShaderModule module;
    };
}
