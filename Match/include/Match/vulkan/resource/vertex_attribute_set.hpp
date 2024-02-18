#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    struct MATCH_API InputAttributeInfo {
        InputAttributeInfo(VertexType type, uint32_t stride = 0) : type(type), stride(0) {}
        VertexType type;
        uint32_t stride;
    };

    struct MATCH_API InputBindingInfo {
        uint32_t binding;
        InputRate rate;
        std::vector<InputAttributeInfo> attributes;
    };

    class MATCH_API VertexAttributeSet {
        no_copy_move_construction(VertexAttributeSet)
    public:
        VertexAttributeSet(const std::vector<InputBindingInfo> &binding_infos);
        ~VertexAttributeSet();
    private:
        void add_input_binding(const InputBindingInfo &binding_info);
    INNER_VISIBLE:
        uint32_t location = 0;
        std::vector<vk::VertexInputAttributeDescription> attributes;
        std::vector<vk::VertexInputBindingDescription> bindings;
    };
}
