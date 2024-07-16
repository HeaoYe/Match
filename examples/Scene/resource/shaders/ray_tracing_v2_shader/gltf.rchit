#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

#include <MatchTypes>
#include "compute_hit_color.glsl"

layout (binding = 5, scalar) readonly buffer MatchInstanceAddressInfo_ { MatchInstanceAddressInfo infos[]; } addr_info_;
layout (binding = 7, scalar) readonly buffer MatchGLTFPrimitiveInstanceDataBuffer { MatchGLTFPrimitiveInstanceData primitive_datas[]; } primitive_;
layout (binding = 8, scalar) readonly buffer MatchGLTFMaterialBuffer { MatchGLTFMaterial materials[]; } gltf_material_;
layout (binding = 9, scalar) readonly buffer GLTFNormal { vec3 normals[]; } normal_;
layout (binding = 10, scalar) readonly buffer GLTFTexCoord { vec2 tex_coords[]; } tex_coord_;
layout (binding = 11) uniform sampler2D textures[];
layout (buffer_reference, scalar) readonly buffer VertexBuffer { vec3 vertices[]; };
layout (buffer_reference, scalar) readonly buffer IndexBuffer { MatchIndex indices[]; };

hitAttributeEXT vec3 attribs;

const float PI = 3.1415926535;

// 反射强度函数
vec3 F(vec3 F0, float cos_theta) {
    return F0 + (1 - F0) * pow(1 - cos_theta, 5);
}

// 法线分布函数
float NDF_GGX(vec3 N, vec3 M, float roughness2) {
    float NoM = dot(N, M);
    if (NoM <= 0) {
        return 0;
    }
    float n = 1 + NoM * NoM * (roughness2 - 1);
    return clamp(roughness2 / (PI * n * n + 0.001), 0, 1);
}

// 几何遮挡函数
// G/4(NoV)(NoL)
float G2(float NoL, float NoV, float roughness2) {
    float lambda_in = NoV * sqrt(roughness2 + NoL * sqrt(NoL - roughness2 * NoL));
    float lambda_out = NoL * sqrt(roughness2 + NoV * sqrt(NoV - roughness2 * NoV));
    return 0.5 / (lambda_in + lambda_out + 0.001);
}

vec3 compute_BRDF_NoL(vec3 direction, vec3 albedo, vec3 N, vec3 V, vec3 F0, float roughness2, float metallic) {
    vec3 H = normalize(V + direction);
    float NoL = max(dot(N, direction), 0);
    float NoV = max(dot(N, V), 0);
    float LoH = max(dot(direction, H), 0);

    vec3 diffuse_brdf = albedo * (1 - metallic) / PI;
    vec3 spec_brdf = F(F0, LoH) * G2(NoL, NoV, roughness2) * NDF_GGX(N, H, roughness2);

    return (diffuse_brdf + spec_brdf) * NoL;
}

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
    float metallic = gltf_material.metallic_factor;
    float roughness = gltf_material.roughness_factor;
    if (gltf_material.base_color_texture > -1) {
        albedo *= texture(textures[nonuniformEXT(gltf_material.base_color_texture)], uv).rgb;
    }
    if (gltf_material.metallic_roughness_texture > -1) {
        vec4 rm = texture(textures[nonuniformEXT(gltf_material.metallic_roughness_texture)], uv);
        metallic *= rm.r;
        roughness *= rm.b;
    }
    vec3 emissive_color = vec3(0);
    if (gltf_material.emissive_texture > -1) {
        vec4 emissive = texture(textures[nonuniformEXT(gltf_material.emissive_texture)], uv);
        emissive_color = emissive.rgb;
    }

    ray.count += 1;

    vec3 random_direction = normalize(vec3(uniform_rnd(ray.rnd_state, 0, 1), uniform_rnd(ray.rnd_state, 0, 1), uniform_rnd(ray.rnd_state, 0, 1)));
    vec3 diff_direction = normalize(normal + random_direction);
    ray.direction = normalize(mix(
        reflect(gl_WorldRayDirectionEXT, normal),
        diff_direction,
        roughness
    ));

    ray.color += ray.albedo * gltf_material.emissive_factor * emissive_color;
    vec3 F0 = albedo * metallic + (vec3(0.04) * (1 - metallic));
    float roughness2 = roughness * roughness;
    vec3 brdf_cosine = compute_BRDF_NoL(ray.direction, albedo, normal, normalize(ray.origin - pos), F0, roughness2, metallic);
    float pdf = 1 / (2 * PI);
    ray.albedo *= brdf_cosine / pdf;

    ray.origin = pos;
}
