#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    struct PushConstantInfo {
        std::string name;
        ConstantType type;
    };

    class PushConstants {
        no_copy_move_construction(PushConstants)
    public:
        MATCH_API PushConstants(ShaderStages stages, const std::vector<PushConstantInfo> &infos);
        MATCH_API void push_constant(const std::string &name, BasicConstantValue basic_value);
        MATCH_API void push_constant(const std::string &name, void *data);
        MATCH_API ~PushConstants();
    INNER_VISIBLE:
        uint32_t constants_size;
        std::vector<uint8_t> constants;
        vk::PushConstantRange range;
        std::map<std::string, uint32_t> constant_size_map;
        std::map<std::string, uint32_t> constant_offset_map;
    };
}
