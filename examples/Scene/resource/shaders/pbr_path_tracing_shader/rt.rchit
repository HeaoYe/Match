#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

#include <MatchTypes>
#include <MatchRandom>

layout (binding = 3, scalar) readonly buffer MatchInstanceAddressInfo_ { MatchInstanceAddressInfo infos[]; } addr_info_;
layout (binding = 4, scalar) readonly buffer MatchGLTFPrimitiveInstanceDataBuffer { MatchGLTFPrimitiveInstanceData primitive_datas[]; } primitive_;
layout (binding = 5, scalar) readonly buffer MatchGLTFMaterialBuffer { MatchGLTFMaterial materials[]; } gltf_material_;
layout (binding = 6, scalar) readonly buffer GLTFNormal { vec3 normals[]; } normal_;
layout (buffer_reference, scalar) readonly buffer VertexBuffer { vec3 vertices[]; };
layout (buffer_reference, scalar) readonly buffer IndexBuffer { MatchIndex indices[]; };

layout (push_constant) uniform C {
    float time;
    int sample_count;
    int frame_count;
    float roughness;
    float metallic;
    vec3 F0;
    float light_intensity;
    float prob;
} c;

struct RayInfo {
    bool end;
    uint rnd_state;
    vec3 color;
    vec3 albedo;
    vec3 origin;
    vec3 direction;
};

layout (binding = 1) uniform accelerationStructureEXT instance;

layout (location = 0) rayPayloadInEXT RayInfo ray;
hitAttributeEXT vec3 attribs;

// 高斯分布随机数
float uniform_rnd(inout uint state, float mean, float std) {
    return mean + std * (sqrt(-2 * log(rnd(state))) * cos(2 * 3.1415926535 * rnd(state)));
}

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

    vec3 W = vec3(1 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 pos = W.x * v0 + W.y * v1 + W.z * v2;
    pos = (gl_ObjectToWorldEXT * vec4(pos, 1)).xyz;
    vec3 normal = normalize(W.x * n0 + W.y * n1 + W.z * n2);
    normal = normalize((gl_ObjectToWorldEXT * vec4(normal, 0)).xyz);

    float roughness = c.roughness * c.roughness;
    float roughness2 = roughness * roughness;

    vec3 random_direction = normalize(vec3(uniform_rnd(ray.rnd_state, 0, 1), uniform_rnd(ray.rnd_state, 0, 1), uniform_rnd(ray.rnd_state, 0, 1)));
    // 这里不是随机采样,而是在法线方向的正态分布采样,为简化pdf的计算,近似认为是随机采样
    vec3 diff_direction = normalize(normal + random_direction);
    ray.direction = normalize(mix(
        reflect(gl_WorldRayDirectionEXT, normal),
        diff_direction,
        roughness
    ));

    // 俄罗斯转盘赌算法,每条光线有c.prop的几率继续跟踪
    ray.end = rnd(ray.rnd_state) > c.prob;

    ray.color += ray.albedo * gltf_material.emissive_factor * c.light_intensity;

    // 计算BRDF * max(dot(normal, light), 0)
    vec3 brdf_cosine = compute_BRDF_NoL(ray.direction, gltf_material.base_color_factor.rgb, normal, normalize(ray.origin - pos), c.F0, roughness2, c.metallic);
    // 对于随机采样,pdf函数值恒为1 / (2 * PI)
    float pdf = 1 / (2 * PI);
    // 因为只有c.prob的光线,所以要除以c.prob
    ray.albedo *= brdf_cosine / pdf / c.prob;

    ray.origin = pos;
}
