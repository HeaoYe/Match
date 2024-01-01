#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    struct InputAttributeInfo {
        InputAttributeInfo(VertexType type, uint32_t stride = 0) : type(type), stride(0) {}
        VertexType type;
        uint32_t stride;
    };

    struct InputBindingInfo {
        uint32_t binding;
        InputRate rate;
        std::vector<InputAttributeInfo> attributes;
    };

    class  VertexAttributeSet {
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
