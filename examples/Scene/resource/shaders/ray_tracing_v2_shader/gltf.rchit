#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require

#include <MatchTypes>
#include "ray_tracing_v2_shader/compute_hit_color.glsl"

layout (binding = 5, scalar) buffer MatchInstanceAddressInfo_ { MatchInstanceAddressInfo infos[]; } addr_info_;
layout (binding = 6, scalar) buffer Material_ { Material materials[]; } material_;
layout (buffer_reference, scalar) buffer VertexBuffer { vec3 vertices[]; };
layout (buffer_reference, scalar) buffer IndexBuffer { MatchIndex indices[]; };

hitAttributeEXT vec3 attribs;

void main() {
    MatchInstanceAddressInfo address_info = addr_info_.infos[gl_InstanceCustomIndexEXT];
    Material material = material_.materials[gl_InstanceCustomIndexEXT];
    VertexBuffer vertex_bufer = VertexBuffer(address_info.vertex_buffer_address);
    IndexBuffer index_buffer = IndexBuffer(address_info.index_buffer_address);
    
    MatchIndex idx = index_buffer.indices[gl_PrimitiveID];
    vec3 v0 = vertex_bufer.vertices[idx.i0];
    vec3 v1 = vertex_bufer.vertices[idx.i1];
    vec3 v2 = vertex_bufer.vertices[idx.i2];
    
    vec3 W = vec3(1 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 pos = W.x * v0 + W.y * v1 + W.z * v2;
    pos = (gl_ObjectToWorldEXT * vec4(pos, 1)).xyz;
    vec3 normal = normalize(cross(v0 - v1, v1 - v2));
    normal = normalize((gl_ObjectToWorldEXT * vec4(normal, 0)).xyz);

    compute_hit_color(pos, normal, material);
}
