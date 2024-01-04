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
        Shader(const std::string &name, const std::string &code, ShaderStage stage);
        Shader(const std::vector<char> &code);
        Shader &bind_push_constants(const std::vector<ConstantInfo> &constant_infos);
        bool is_ready();
        ~Shader();
    private:
        void create(const uint32_t *data, uint32_t size);
    INNER_VISIBLE:
        std::optional<vk::ShaderModule> module;
        std::optional<uint32_t> first_align;
        uint32_t constants_size;
        std::map<std::string, uint32_t> constant_size_map;
        std::map<std::string, uint32_t> constant_offset_map;
    };
}
