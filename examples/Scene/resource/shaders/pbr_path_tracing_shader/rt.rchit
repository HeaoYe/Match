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
    // 确保不会0/0
    if (roughness2 < 1e-3) {
        return 1;
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

    // cosine重要性采样，得到局部空间的采样方向
    float r = sqrt(rnd(ray.rnd_state));
    float theta = 2 * PI * rnd(ray.rnd_state);
    vec3 cosine_direction_local = vec3(r * cos(theta), sqrt(1 - r * r), r * sin(theta));
    // 将局部空间转为世界空间
    vec3 up = abs(normal.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 X = normalize(cross(up, normal));
    vec3 Z = normalize(cross(X, normal));
    vec3 cosine_direction = normalize(X * cosine_direction_local.x + normal * cosine_direction_local.y + Z * cosine_direction_local.z);
    // cosine重要性采样的pdf由出射方向与法线夹角的cos得出
    // 因为夹角的cosine值在出射半球上的积分为PI，所以pdf=cosine/PI
    float cosine_pdf = dot(cosine_direction, normal) / PI;

    // 对于完美的漫反射，cosine重要性采样可以很好的降低方差
    // 但对于roughness不为1的物体，也就是光滑的物体
    // 越光滑，cosine重要性采样的效果越差
    // 所以同样使用完美镜面反射采样一个出射方向
    vec3 reflect_direction = reflect(ray.direction, normal);
    // 对于完美镜面反射，他的pdf为狄拉克delta分布
    // 对应的brdf也是狄拉克delta分布，即 取值无限大(?)
    // 所以上下抵消，可将pdf设为1
    float reflect_pdf = 1;

    // 最后根据roughness的值，得到最终的 采样方向 和 pdf
    ray.direction = normalize(mix(
        reflect_direction,
        cosine_direction,
        roughness
    ));

    float pdf = mix(
        reflect_pdf,
        cosine_pdf,
        roughness
    );

    // 俄罗斯转盘赌算法,每条光线有c.prop的几率继续跟踪
    ray.end = rnd(ray.rnd_state) > c.prob;

    ray.color += ray.albedo * gltf_material.emissive_factor * c.light_intensity;

    // 计算BRDF * max(dot(normal, light), 0)
    vec3 brdf_cosine = compute_BRDF_NoL(ray.direction, gltf_material.base_color_factor.rgb, normal, normalize(ray.origin - pos), c.F0, roughness2, c.metallic);
    // 因为只有c.prob的光线,所以要除以c.prob
    ray.albedo *= brdf_cosine / pdf / c.prob;

    ray.origin = pos;
}
