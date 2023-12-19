#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class Shader {
        no_copy_move_construction(Shader)
    public:
        Shader(const std::vector<char> &code);
        Shader(const std::vector<uint32_t> &code);
        ~Shader();
    private:
        void create(const uint32_t *data, uint32_t size);
    INNER_VISIBLE:
        VkShaderModule module;
    };
}
