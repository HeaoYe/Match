#pragma once

#include <Match/vulkan/commons.hpp>
#include <Match/vulkan/resource/vertex_attribute_set.hpp>
#include <optional>

namespace Match {
    struct MATCH_API ConstantInfo {
        std::string name;
        ConstantType type;
    };

    class MATCH_API Shader {
        no_copy_move_construction(Shader)
        using binding = uint32_t;
    public:
        Shader(const std::string &name, const std::string &code, ShaderStage stage);
        Shader(const std::vector<char> &code);
        bool is_ready();
        ~Shader();
    private:
        void create(const uint32_t *data, uint32_t size);
    INNER_VISIBLE:
        std::optional<vk::ShaderModule> module;
    };
}
