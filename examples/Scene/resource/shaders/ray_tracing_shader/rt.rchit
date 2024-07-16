#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require

// Match内置的类型头文件
// Match内置的类型统一改为以Match开头
#include <MatchTypes>

struct MyCustomData1 {
    vec3 color;
};

struct MyCustomData2 {
    float color_scale;
};

layout (binding = 1) uniform accelerationStructureEXT instance;
layout (binding = 3, scalar) buffer MatchInstanceAddressInfo_ { MatchInstanceAddressInfo infos[]; } addr_info_;
layout (binding = 4, scalar) buffer MyCustomData1_ { MyCustomData1 datas[]; } data1_info_;
layout (binding = 5, scalar) buffer MyCustomData2_ { MyCustomData2 datas[]; } data2_info_;
layout (buffer_reference, scalar) buffer VertexBuffer { MatchVertex vertices[]; };
layout (buffer_reference, scalar) buffer IndexBuffer { MatchIndex indices[]; };

layout (push_constant) uniform Constants {
    vec3 sky_color;
    vec3 pos;
    vec3 color;
    float intensity;
    int sample_count;
} constant;

layout (location = 0) rayPayloadInEXT vec3 ray_color;
layout (location = 1) rayPayloadEXT bool is_in_shadow;

// 命中点在三角形中的位置信息
hitAttributeEXT vec3 attribs;

void main() {
    // 获取命中的模型的信息
    MatchInstanceAddressInfo address_info = addr_info_.infos[gl_InstanceCustomIndexEXT];
    MyCustomData1 data1 = data1_info_.datas[gl_InstanceCustomIndexEXT];
    MyCustomData2 data2 = data2_info_.datas[gl_InstanceCustomIndexEXT];

    VertexBuffer vertex_bufer = VertexBuffer(address_info.vertex_buffer_address);
    IndexBuffer index_buffer = IndexBuffer(address_info.index_buffer_address);

    // 获取命中的三角形的顶点信息
    MatchIndex idx = index_buffer.indices[gl_PrimitiveID];
    MatchVertex v0 = vertex_bufer.vertices[idx.i0];
    MatchVertex v1 = vertex_bufer.vertices[idx.i1];
    MatchVertex v2 = vertex_bufer.vertices[idx.i2];

    // 获取命中点的信息(位置,法向量)
    vec3 W = vec3(1 - attribs.x - attribs.y, attribs.x, attribs.y);
    // 获取模型坐标系下的位置
    vec3 pos = W.x * v0.pos + W.y * v1.pos + W.z * v2.pos;
    // 转换到世界坐标系
    pos = (gl_ObjectToWorldEXT * vec4(pos, 1)).xyz;
    // 获取模型坐标系下的法向量
    vec3 normal = W.x * v0.normal + W.y * v1.normal + W.z * v2.normal;
    // 转换到世界坐标系
    normal = (gl_ObjectToWorldEXT * vec4(normal, 0)).xyz;

    // 计算光照颜色
    vec3 L = normalize(constant.pos - pos);
    float D = length(constant.pos - pos);
    // 舍弃了一点光照的正确,换来一点亮度
    ray_color = vec3(data1.color) * data2.color_scale + (1 - data2.color_scale) * constant.color * max(dot(L, normal), 0) * constant.intensity / (D);

    // 计算阴影
    is_in_shadow = true;
    // 光线信息         第一次检测到命中就结束该光线的追踪           不透明                    不调用最近命中着色器
    uint ray_flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    // 从pos向光源发射光线,若未命中,则说明不在阴影中
    traceRayEXT(
        // 模型信息
        instance,
        // 光线信息
        ray_flags,
        0xff,
        0, 0,
        // 未命中着色器索引
        1,
        // 光线起点
        pos,
        // 最小距离
        0.00001,
        // 光线方向
        L,
        // 最大距离
        D,
        // rayPayload location = 1
        // 不同的光追着色器使用rayPayload传递数据
        1
    );
    if (is_in_shadow) {
        ray_color *= 0.3;
    }
}
