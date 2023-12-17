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
}
