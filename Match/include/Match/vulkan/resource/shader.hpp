#pragma once

#include <Match/vulkan/commons.hpp>
#include <Match/vulkan/resource/vertex_attribute_set.hpp>
#include <optional>

namespace Match {
    struct ConstantInfo {
        std::string name;
        ConstantType type;
    };

    class Shader {
        no_copy_move_construction(Shader)
        using binding = uint32_t;
    public:
        MATCH_API Shader(const std::string &name, const std::vector<char> &code, ShaderStage stage);
        MATCH_API Shader(const std::vector<char> &code);
        MATCH_API bool is_ready();
        MATCH_API ~Shader();
    private:
        MATCH_API void create(const uint32_t *data, uint32_t size);
    INNER_VISIBLE:
        std::optional<vk::ShaderModule> module;
    };
}
