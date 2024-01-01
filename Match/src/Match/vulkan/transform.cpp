#include <Match/core/utils.hpp>
#include <Match/vulkan/resource/vertex_attribute_set.hpp>
#include <Match/vulkan/resource/buffer.hpp>
#include <Match/vulkan/resource/shader_program.hpp>
#include <Match/vulkan/resource/sampler.hpp>

namespace Match {
    #define __case(R, Y, e) case Y::e: return R;
    #define _case(T, d, Y, e) __case(T::d, Y, e);
    template <>
    vk::PrimitiveTopology transform<vk::PrimitiveTopology>(Topology topology) {
        switch (topology) {
        _case(vk::PrimitiveTopology, ePointList, Topology, ePointList)
        _case(vk::PrimitiveTopology, eLineList, Topology, eLineList)
        _case(vk::PrimitiveTopology, eLineStrip, Topology, eLineStrip)
        _case(vk::PrimitiveTopology, eTriangleList, Topology, eTriangleList)
        _case(vk::PrimitiveTopology, eTriangleFan, Topology, eTriangleFan)
        }
    }

    template <>
    vk::PolygonMode transform<vk::PolygonMode>(PolygonMode mode) {
        switch (mode) {
        _case(vk::PolygonMode, eFill, PolygonMode, eFill)
        _case(vk::PolygonMode, eLine, PolygonMode, eLine)
        _case(vk::PolygonMode, ePoint, PolygonMode, ePoint)
        }
    }

    template <>
    vk::CullModeFlags transform<vk::CullModeFlags>(CullMode mode) {
        switch (mode) {
        _case(vk::CullModeFlagBits, eNone, CullMode, eNone)
        _case(vk::CullModeFlagBits, eFront, CullMode, eFront)
        _case(vk::CullModeFlagBits, eBack, CullMode, eBack)
        _case(vk::CullModeFlagBits, eFrontAndBack, CullMode, eFrontAndBack)
        }
    }

    template <>
    vk::FrontFace transform<vk::FrontFace>(FrontFace mode) {
        switch (mode) {
        _case(vk::FrontFace, eClockwise, FrontFace, eClockwise)
        _case(vk::FrontFace, eCounterClockwise, FrontFace, eCounterClockwise)
        }
    }

    template <>
    vk::Format transform<vk::Format>(VertexType type) {
        switch (type) {
        _case(vk::Format, eR8Sint, VertexType, eInt8)
        _case(vk::Format, eR8G8Sint, VertexType, eInt8x2)
        _case(vk::Format, eR8G8B8Sint, VertexType, eInt8x3)
        _case(vk::Format, eR8G8B8A8Sint, VertexType, eInt8x4)
        _case(vk::Format, eR16Sint, VertexType, eInt16)
        _case(vk::Format, eR16G16Sint, VertexType, eInt16x2)
        _case(vk::Format, eR16G16B16Sint, VertexType, eInt16x3)
        _case(vk::Format, eR16G16B16A16Sint, VertexType, eInt16x4)
        _case(vk::Format, eR32Sint, VertexType, eInt32)
        _case(vk::Format, eR32G32Sint, VertexType, eInt32x2)
        _case(vk::Format, eR32G32B32Sint, VertexType, eInt32x3)
        _case(vk::Format, eR32G32B32A32Sint, VertexType, eInt32x4)
        _case(vk::Format, eR64Sint, VertexType, eInt64)
        _case(vk::Format, eR64G64Sint, VertexType, eInt64x2)
        _case(vk::Format, eR64G64B64Sint, VertexType, eInt64x3)
        _case(vk::Format, eR64G64B64A64Sint, VertexType, eInt64x4)
        _case(vk::Format, eR8Uint, VertexType, eUint8)
        _case(vk::Format, eR8G8Uint, VertexType, eUint8x2)
        _case(vk::Format, eR8G8B8Uint, VertexType, eUint8x3)
        _case(vk::Format, eR8G8B8A8Uint, VertexType, eUint8x4)
        _case(vk::Format, eR16Uint, VertexType, eUint16)
        _case(vk::Format, eR16G16Uint, VertexType, eUint16x2)
        _case(vk::Format, eR16G16B16Uint, VertexType, eUint16x3)
        _case(vk::Format, eR16G16B16A16Uint, VertexType, eUint16x4)
        _case(vk::Format, eR32Uint, VertexType, eUint32)
        _case(vk::Format, eR32G32Uint, VertexType, eUint32x2)
        _case(vk::Format, eR32G32B32Uint, VertexType, eUint32x3)
        _case(vk::Format, eR32G32B32A32Uint, VertexType, eUint32x4)
        _case(vk::Format, eR64Uint, VertexType, eUint64)
        _case(vk::Format, eR64G64Uint, VertexType, eUint64x2)
        _case(vk::Format, eR64G64B64Uint, VertexType, eUint64x3)
        _case(vk::Format, eR64G64B64A64Uint, VertexType, eUint64x4)
        _case(vk::Format, eR32Sfloat, VertexType, eFloat)
        _case(vk::Format, eR32G32Sfloat, VertexType, eFloat2)
        _case(vk::Format, eR32G32B32Sfloat, VertexType, eFloat3)
        _case(vk::Format, eR32G32B32A32Sfloat, VertexType, eFloat4)
        _case(vk::Format, eR64Sfloat, VertexType, eDouble)
        _case(vk::Format, eR64G64Sfloat, VertexType, eDouble2)
        _case(vk::Format, eR64G64B64Sfloat, VertexType, eDouble3)
        _case(vk::Format, eR64G64B64A64Sfloat, VertexType, eDouble4)
        }
    }

    template <>
    uint32_t transform<uint32_t>(VertexType type) {
        switch (type) {
        __case(1, VertexType, eInt8)
        __case(2, VertexType, eInt8x2)
        __case(3, VertexType, eInt8x3)
        __case(4, VertexType, eInt8x4)
        __case(2, VertexType, eInt16)
        __case(4, VertexType, eInt16x2)
        __case(6, VertexType, eInt16x3)
        __case(8, VertexType, eInt16x4)
        __case(4, VertexType, eInt32)
        __case(8, VertexType, eInt32x2)
        __case(12, VertexType, eInt32x3)
        __case(16, VertexType, eInt32x4)
        __case(8, VertexType, eInt64)
        __case(16, VertexType, eInt64x2)
        __case(24, VertexType, eInt64x3)
        __case(32, VertexType, eInt64x4)
        __case(1, VertexType, eUint8)
        __case(2, VertexType, eUint8x2)
        __case(3, VertexType, eUint8x3)
        __case(4, VertexType, eUint8x4)
        __case(2, VertexType, eUint16)
        __case(4, VertexType, eUint16x2)
        __case(6, VertexType, eUint16x3)
        __case(8, VertexType, eUint16x4)
        __case(4, VertexType, eUint32)
        __case(8, VertexType, eUint32x2)
        __case(12, VertexType, eUint32x3)
        __case(16, VertexType, eUint32x4)
        __case(8, VertexType, eUint64)
        __case(16, VertexType, eUint64x2)
        __case(24, VertexType, eUint64x3)
        __case(32, VertexType, eUint64x4)
        __case(4, VertexType, eFloat)
        __case(8, VertexType, eFloat2)
        __case(12, VertexType, eFloat3)
        __case(16, VertexType, eFloat4)
        __case(8, VertexType, eDouble)
        __case(16, VertexType, eDouble2)
        __case(24, VertexType, eDouble3)
        __case(32, VertexType, eDouble4)
        }
    }

    template <>
    vk::VertexInputRate transform<vk::VertexInputRate>(InputRate rate) {
        switch (rate) {
        _case(vk::VertexInputRate, eVertex, InputRate, ePerVertex)
        _case(vk::VertexInputRate, eInstance, InputRate, ePerInstance)
        }
    }

    template <>
    vk::IndexType transform<vk::IndexType>(IndexType type) {
        switch (type) {
        _case(vk::IndexType, eUint16, IndexType, eUint16)
        _case(vk::IndexType, eUint32, IndexType, eUint32)
        }
    }

    template <>
    uint32_t transform<uint32_t>(IndexType type) {
        switch (type) {
        __case(2, IndexType, eUint16)
        __case(4, IndexType, eUint32)
        }
    }

    template <>
    vk::DescriptorType transform<vk::DescriptorType>(DescriptorType type) {
        switch (type) {
        _case(vk::DescriptorType, eUniformBuffer, DescriptorType, eUniform)
        _case(vk::DescriptorType, eCombinedImageSampler, DescriptorType, eTexture)
        _case(vk::DescriptorType, eCombinedImageSampler, DescriptorType, eTextureAttachment)
        _case(vk::DescriptorType, eInputAttachment, DescriptorType, eInputAttachment)
        }
    }

    template <>
    vk::Filter transform<vk::Filter>(SamplerFilter filter) {
        switch (filter) {
        _case(vk::Filter, eLinear, SamplerFilter, eLinear)
        _case(vk::Filter, eNearest, SamplerFilter, eNearest)
        }
    }

    template <>
    vk::SamplerMipmapMode transform<vk::SamplerMipmapMode>(SamplerFilter filter) {
        switch (filter) {
        _case(vk::SamplerMipmapMode, eLinear, SamplerFilter, eLinear)
        _case(vk::SamplerMipmapMode, eNearest, SamplerFilter, eNearest)
        }
    }

    template <>
    vk::SamplerAddressMode transform<vk::SamplerAddressMode>(SamplerAddressMode mode) {
        switch (mode) {
        _case(vk::SamplerAddressMode, eClampToBorder, SamplerAddressMode, eClampToBorder)
        _case(vk::SamplerAddressMode, eClampToEdge, SamplerAddressMode, eClampToEdge)
        _case(vk::SamplerAddressMode, eRepeat, SamplerAddressMode, eRepeat)
        _case(vk::SamplerAddressMode, eMirroredRepeat, SamplerAddressMode, eMirroredRepeat)
        }
    }

    template <>
    vk::BorderColor transform<vk::BorderColor>(SamplerBorderColor color) {
        switch (color) {
        _case(vk::BorderColor, eFloatOpaqueBlack, SamplerBorderColor, eFloatOpaqueBlack)
        _case(vk::BorderColor, eFloatOpaqueWhite, SamplerBorderColor, eFloatOpaqueWhite)
        _case(vk::BorderColor, eFloatTransparentBlack, SamplerBorderColor, eFloatTransparentBlack)
        _case(vk::BorderColor, eIntOpaqueBlack, SamplerBorderColor, eIntOpaqueBlack)
        _case(vk::BorderColor, eIntOpaqueWhite, SamplerBorderColor, eIntOpaqueWhite)
        _case(vk::BorderColor, eIntTransparentBlack, SamplerBorderColor, eIntTransparentBlack)
        }
    }

    template <>
    vk::SampleCountFlagBits transform<vk::SampleCountFlagBits>(SampleCount count) {
        switch (count) {
        _case(vk::SampleCountFlagBits, e1, SampleCount, e1)
        _case(vk::SampleCountFlagBits, e2, SampleCount, e2)
        _case(vk::SampleCountFlagBits, e4, SampleCount, e4)
        _case(vk::SampleCountFlagBits, e8, SampleCount, e8)
        _case(vk::SampleCountFlagBits, e16, SampleCount, e16)
        _case(vk::SampleCountFlagBits, e32, SampleCount, e32)
        _case(vk::SampleCountFlagBits, e64, SampleCount, e64)
        }
    }
}
