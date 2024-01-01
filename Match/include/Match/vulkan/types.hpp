#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    enum class AttachmentType {
        eColor,
        eDepth,
        eDepthBuffer,
        eFloat4Buffer,
    };

    struct AccessInfo {
        vk::PipelineStageFlags stage;
        vk::AccessFlags access;
    };

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
    
    enum class IndexType {
        eUint16,
        eUint32,
    };

    typedef VertexType ConstantType;

    enum class SamplerFilter {
        eNearest,
        eLinear,
    };

    enum class SamplerAddressMode {
        eRepeat,
        eMirroredRepeat,
        eClampToEdge,
        eClampToBorder,
    };

    enum class SamplerBorderColor {
        eFloatTransparentBlack,
        eIntTransparentBlack,
        eFloatOpaqueBlack,
        eIntOpaqueBlack,
        eFloatOpaqueWhite,
        eIntOpaqueWhite,
    };
    enum class Topology {
        ePointList,
        eLineList,
        eLineStrip,
        eTriangleList,
        eTriangleFan,
    };
    
    enum class PolygonMode {
        eFill,
        eLine,
        ePoint,
    };

    enum class CullMode {
        eNone,
        eFront,
        eBack,
        eFrontAndBack,
    };

    enum class FrontFace {
        eClockwise,
        eCounterClockwise,
    };

    enum class ShaderType {
        eCompiled,
        eVertexShaderNeedCompile,
        eFragmentShaderNeedCompile,
    };
    
    enum class InputRate {
        ePerVertex,
        ePerInstance,
    };

    enum class DescriptorType {
        eUniform,
        eTexture,
        eTextureAttachment,
        eInputAttachment,
    };

    union BasicConstantValue {
        bool b;
        BasicConstantValue(bool b) : b(b) {}
        int32_t i;
        BasicConstantValue(int32_t i) : i(i) {}
        uint32_t ui;
        BasicConstantValue(uint32_t ui) : ui(ui) {}
        float f;
        BasicConstantValue(float f) : f(f) {}
    };
}
