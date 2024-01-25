#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

#include <MatchTypes>
#include "ray_tracing_v2_shader/compute_hit_color.glsl"

layout (binding = 5, scalar) readonly buffer MatchInstanceAddressInfo_ { MatchInstanceAddressInfo infos[]; } addr_info_;
layout (binding = 7, scalar) readonly buffer MatchGLTFPrimitiveInstanceDataBuffer { MatchGLTFPrimitiveInstanceData primitive_datas[]; } primitive_;
layout (binding = 8, scalar) readonly buffer MatchGLTFMaterialBuffer { MatchGLTFMaterial materials[]; } gltf_material_;
layout (binding = 9, scalar) readonly buffer GLTFNormal { vec3 normals[]; } normal_;
layout (binding = 10, scalar) readonly buffer GLTFTexCoord { vec2 tex_coords[]; } tex_coord_;
layout (binding = 11) uniform sampler2D textures[];
layout (buffer_reference, scalar) readonly buffer VertexBuffer { vec3 vertices[]; };
layout (buffer_reference, scalar) readonly buffer IndexBuffer { MatchIndex indices[]; };

hitAttributeEXT vec3 attribs;

void main() {
    MatchInstanceAddressInfo address_info = addr_info_.infos[gl_InstanceCustomIndexEXT];
    MatchGLTFPrimitiveInstanceData primitive = primitive_.primitive_datas[gl_InstanceCustomIndexEXT];
    
    VertexBuffer vertex_bufer = VertexBuffer(address_info.vertex_buffer_address);
    IndexBuffer index_buffer = IndexBuffer(address_info.index_buffer_address);
    
    MatchGLTFMaterial gltf_material = gltf_material_.materials[primitive.material_index];
    MatchIndex idx = index_buffer.indices[primitive.first_index / 3 + gl_PrimitiveID];
    idx.i0 += primitive.first_vertex;
    idx.i1 += primitive.first_vertex;
    idx.i2 += primitive.first_vertex;
    const vec3 v0 = vertex_bufer.vertices[idx.i0];
    const vec3 v1 = vertex_bufer.vertices[idx.i1];
    const vec3 v2 = vertex_bufer.vertices[idx.i2];
    const vec3 n0 = normal_.normals[idx.i0];
    const vec3 n1 = normal_.normals[idx.i1];
    const vec3 n2 = normal_.normals[idx.i2];
    const vec2 uv0 = tex_coord_.tex_coords[idx.i0];
    const vec2 uv1 = tex_coord_.tex_coords[idx.i1];
    const vec2 uv2 = tex_coord_.tex_coords[idx.i2];
    
    vec3 W = vec3(1 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 pos = W.x * v0 + W.y * v1 + W.z * v2;
    pos = (gl_ObjectToWorldEXT * vec4(pos, 1)).xyz;
    vec3 normal = normalize(W.x * n0 + W.y * n1 + W.z * n2);
    normal = normalize((gl_ObjectToWorldEXT * vec4(normal, 0)).xyz);
    vec2 uv = W.x * uv0 + W.y * uv1 + W.z * uv2;

    vec3 albedo = gltf_material.base_color_factor.rgb;
    float matellic = gltf_material.metallic_factor;
    float roughness = gltf_material.roughness_factor;
    if (gltf_material.base_color_texture > -1) {
        albedo *= texture(textures[nonuniformEXT(gltf_material.base_color_texture)], uv).rgb;
    }
    if (gltf_material.metallic_roughness_texture > -1) {
        vec4 rm = texture(textures[nonuniformEXT(gltf_material.metallic_roughness_texture)], uv);
        matellic *= rm.r;
        roughness *= rm.g;
    }

    ray.count += 1;
    float is_spec = float(matellic >= rnd(ray.rnd_state));
    ray.albedo *= albedo;
    vec3 random_direction = normalize(vec3(uniform_rnd(ray.rnd_state, 0, 1), uniform_rnd(ray.rnd_state, 0, 1), uniform_rnd(ray.rnd_state, 0, 1)));
    vec3 diff_direction = normalize(normal + random_direction);
    ray.direction = normalize(mix(
        reflect(gl_WorldRayDirectionEXT, normal),
        diff_direction,
        roughness * (1 - is_spec)
    ));
    ray.origin = pos;
}
