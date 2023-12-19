#include <Match/core/utils.hpp>
#include <Match/vulkan/resource/shader_program.hpp>

namespace Match {
    #define _case(D, T, e) case T::e: return D;
    template <>
    VkPrimitiveTopology transform<VkPrimitiveTopology>(Topology topology) {
        switch (topology) {
        _case(VK_PRIMITIVE_TOPOLOGY_POINT_LIST, Topology, ePointList)
        _case(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, Topology, eLineList)
        _case(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, Topology, eLineStrip)
        _case(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, Topology, eTriangleList)
        _case(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, Topology, eTriangleFan)
        }
    }

    template <>
    VkPolygonMode transform<VkPolygonMode>(PolygonMode mode) {
        switch (mode) {
        _case(VK_POLYGON_MODE_FILL, PolygonMode, eFill)
        _case(VK_POLYGON_MODE_LINE, PolygonMode, eLine)
        _case(VK_POLYGON_MODE_POINT, PolygonMode, ePoint)
        }
    }

    template <>
    VkCullModeFlags transform<VkCullModeFlags>(CullMode mode) {
        switch (mode) {
        _case(VK_CULL_MODE_NONE, CullMode, eNone)
        _case(VK_CULL_MODE_FRONT_BIT, CullMode, eFront)
        _case(VK_CULL_MODE_BACK_BIT, CullMode, eBack)
        _case(VK_CULL_MODE_FRONT_AND_BACK, CullMode, eFrontAndBack)
        }
    }

    template <>
    VkFrontFace transform<VkFrontFace>(FrontFace mode) {
        switch (mode) {
        _case(VK_FRONT_FACE_COUNTER_CLOCKWISE, FrontFace, eClockwise)
        _case(VK_FRONT_FACE_CLOCKWISE, FrontFace, eCounterClockwise)
        }
    }

    template <>
    VkFormat transform<VkFormat>(VertexType type) {
        switch (type) {
        _case(VK_FORMAT_R8_SINT, VertexType, eInt8)
        _case(VK_FORMAT_R8G8_SINT, VertexType, eInt8x2)
        _case(VK_FORMAT_R8G8B8_SINT, VertexType, eInt8x3)
        _case(VK_FORMAT_R8G8B8A8_SINT, VertexType, eInt8x4)
        _case(VK_FORMAT_R16_SINT, VertexType, eInt16)
        _case(VK_FORMAT_R16G16_SINT, VertexType, eInt16x2)
        _case(VK_FORMAT_R16G16B16_SINT, VertexType, eInt16x3)
        _case(VK_FORMAT_R16G16B16A16_SINT, VertexType, eInt16x4)
        _case(VK_FORMAT_R32_SINT, VertexType, eInt32)
        _case(VK_FORMAT_R32G32_SINT, VertexType, eInt32x2)
        _case(VK_FORMAT_R32G32B32_SINT, VertexType, eInt32x3)
        _case(VK_FORMAT_R32G32B32A32_SINT, VertexType, eInt32x4)
        _case(VK_FORMAT_R64_SINT, VertexType, eInt64)
        _case(VK_FORMAT_R64G64_SINT, VertexType, eInt64x2)
        _case(VK_FORMAT_R64G64B64_SINT, VertexType, eInt64x3)
        _case(VK_FORMAT_R64G64B64A64_SINT, VertexType, eInt64x4)
        _case(VK_FORMAT_R8_UINT, VertexType, eUint8)
        _case(VK_FORMAT_R8G8_UINT, VertexType, eUint8x2)
        _case(VK_FORMAT_R8G8B8_UINT, VertexType, eUint8x3)
        _case(VK_FORMAT_R8G8B8A8_UINT, VertexType, eUint8x4)
        _case(VK_FORMAT_R16_UINT, VertexType, eUint16)
        _case(VK_FORMAT_R16G16_UINT, VertexType, eUint16x2)
        _case(VK_FORMAT_R16G16B16_UINT, VertexType, eUint16x3)
        _case(VK_FORMAT_R16G16B16A16_UINT, VertexType, eUint16x4)
        _case(VK_FORMAT_R32_UINT, VertexType, eUint32)
        _case(VK_FORMAT_R32G32_UINT, VertexType, eUint32x2)
        _case(VK_FORMAT_R32G32B32_UINT, VertexType, eUint32x3)
        _case(VK_FORMAT_R32G32B32A32_UINT, VertexType, eUint32x4)
        _case(VK_FORMAT_R64_UINT, VertexType, eUint64)
        _case(VK_FORMAT_R64G64_UINT, VertexType, eUint64x2)
        _case(VK_FORMAT_R64G64B64_UINT, VertexType, eUint64x3)
        _case(VK_FORMAT_R64G64B64A64_UINT, VertexType, eUint64x4)
        _case(VK_FORMAT_R32_SFLOAT, VertexType, eFloat)
        _case(VK_FORMAT_R32G32_SFLOAT, VertexType, eFloat2)
        _case(VK_FORMAT_R32G32B32_SFLOAT, VertexType, eFloat3)
        _case(VK_FORMAT_R32G32B32A32_SFLOAT, VertexType, eFloat4)
        _case(VK_FORMAT_R64_SFLOAT, VertexType, eDouble)
        _case(VK_FORMAT_R64G64_SFLOAT, VertexType, eDouble2)
        _case(VK_FORMAT_R64G64B64_SFLOAT, VertexType, eDouble3)
        _case(VK_FORMAT_R64G64B64A64_SFLOAT, VertexType, eDouble4)
        }
    }

    template <>
    uint32_t transform<uint32_t>(VertexType type) {
        switch (type) {
        _case(1, VertexType, eInt8)
        _case(2, VertexType, eInt8x2)
        _case(3, VertexType, eInt8x3)
        _case(4, VertexType, eInt8x4)
        _case(2, VertexType, eInt16)
        _case(4, VertexType, eInt16x2)
        _case(6, VertexType, eInt16x3)
        _case(8, VertexType, eInt16x4)
        _case(4, VertexType, eInt32)
        _case(8, VertexType, eInt32x2)
        _case(12, VertexType, eInt32x3)
        _case(16, VertexType, eInt32x4)
        _case(8, VertexType, eInt64)
        _case(16, VertexType, eInt64x2)
        _case(24, VertexType, eInt64x3)
        _case(32, VertexType, eInt64x4)
        _case(1, VertexType, eUint8)
        _case(2, VertexType, eUint8x2)
        _case(3, VertexType, eUint8x3)
        _case(4, VertexType, eUint8x4)
        _case(2, VertexType, eUint16)
        _case(4, VertexType, eUint16x2)
        _case(6, VertexType, eUint16x3)
        _case(8, VertexType, eUint16x4)
        _case(4, VertexType, eUint32)
        _case(8, VertexType, eUint32x2)
        _case(12, VertexType, eUint32x3)
        _case(16, VertexType, eUint32x4)
        _case(8, VertexType, eUint64)
        _case(16, VertexType, eUint64x2)
        _case(24, VertexType, eUint64x3)
        _case(32, VertexType, eUint64x4)
        _case(4, VertexType, eFloat)
        _case(8, VertexType, eFloat2)
        _case(12, VertexType, eFloat3)
        _case(16, VertexType, eFloat4)
        _case(8, VertexType, eDouble)
        _case(16, VertexType, eDouble2)
        _case(24, VertexType, eDouble3)
        _case(32, VertexType, eDouble4)
        }
    }

    template <>
    VkVertexInputRate transform<VkVertexInputRate>(InputRate rate) {
        switch (rate) {
        _case(VK_VERTEX_INPUT_RATE_VERTEX, InputRate, ePerVertex)
        _case(VK_VERTEX_INPUT_RATE_INSTANCE, InputRate, ePerInstance)
        }
    }

    template <>
    VkIndexType transform<VkIndexType>(IndexType type) {
        switch (type) {
        _case(VK_INDEX_TYPE_UINT16, IndexType, eUint16)
        _case(VK_INDEX_TYPE_UINT32, IndexType, eUint32)
        }
    }

    template <>
    uint32_t transform<uint32_t>(IndexType type) {
        switch (type) {
        _case(2, IndexType, eUint16)
        _case(4, IndexType, eUint32)
        }
    }

    template <>
    VkDescriptorType transform<VkDescriptorType>(DescriptorType type) {
        switch (type) {
        _case(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, DescriptorType, eUniform)
        }
    }
}
