#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    enum class VertexType {
        eInt8, eInt8x2, eInt8x3, eInt8x4,
        eInt16, eInt16x2, eInt16x3, eInt16x4,
        eInt32, eInt32x2, eInt32x3, eInt32x4,
        eInt64, eInt64x2, eInt64x3, eInt64x4,
        eUint8, eUint8x2, eUint8x3, eUint8x4,
        eUint16, eUint16x2, eUint16x3, eUint16x4,
        eUint32, eUint32x2, eUint32x3, eUint32x4,
        eUint64, eUint64x2, eUint64x3, eUint64x4,
        eFloat, eFloat2, eFloat3, eFloat4,
        eDouble, eDouble2, eDouble3, eDouble4,
    };

    enum class InputRate {
        ePerVertex,
        ePerInstance,
    };

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
        std::vector<VkVertexInputAttributeDescription> attributes;
        std::vector<VkVertexInputBindingDescription> bindings;
    };
}
