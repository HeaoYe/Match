#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_buffer_reference2 : enable

#include <MatchTypes>

layout (binding = 0, set = 0) uniform accelerationStructureEXT tlas;

struct hitPayload {
    vec3 hit_value;
};

layout (location = 0) rayPayloadInEXT hitPayload prd;
layout (location = 1) rayPayloadEXT bool is_shadow;

layout (binding = 2, set = 0, scalar) buffer InstanceInfo_ { MatchInstanceAddressInfo infos[]; } info_;

layout (binding = 2, set = 1) uniform Light_ {
    vec3 pos;
    vec3 color;
    float intensity;
} light;

layout(buffer_reference, scalar) buffer Vertices { MatchVertex vertices[]; };
layout(buffer_reference, scalar) buffer Indices { MatchIndex indices[]; };

hitAttributeEXT vec3 attribs;

void main() {
    MatchInstanceAddressInfo info = info_.infos[gl_InstanceCustomIndexEXT];

    Vertices vertices = Vertices(info.vertex_buffer_address);
    Indices indices = Indices(info.index_buffer_address);
    MatchIndex idxs = indices.indices[gl_PrimitiveID];
    vec3 W = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    MatchVertex v0 = vertices.vertices[idxs.i0];
    MatchVertex v1 = vertices.vertices[idxs.i1];
    MatchVertex v2 = vertices.vertices[idxs.i2];

    vec3 pos = v0.pos * W.x + v1.pos * W.y + v2.pos * W.z;
    vec3 normal = normalize(v0.normal * W.x + v1.normal * W.y + v2.normal * W.z);

    vec3 L = normalize(light.pos - pos);
    float L_D = length(light.pos - pos);

    is_shadow = true;
    uint ray_flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    traceRayEXT(
        tlas,
        ray_flags,
        0xff,
        0, 0,
        1,
        pos,
        0.0001,
        L,
        L_D,
        1
    );

    prd.hit_value = max(dot(L, normal), 0) * light.color * light.intensity / L_D / L_D;

    if (is_shadow) {
        prd.hit_value *= 0.3;
    }    
}
