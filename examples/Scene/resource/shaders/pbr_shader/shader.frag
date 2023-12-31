#version 450

layout (location = 0) in vec3 frag_pos;
layout (location = 1) in vec3 frag_normal;
layout (location = 2) in vec3 frag_color;

layout (location = 0) out vec4 out_color;

layout (binding = 0) uniform CameraUniform {
    vec3 pos;
    mat4 view;
    mat4 project;
} camera;

layout (binding = 1) uniform PBRMaterial {
    vec3 color;          // 颜色
    float roughness;     // 粗糙度
    float metallic;      // 金属度
    float reflectance;   // 反射度
} material;

struct PointLight {
    vec3 pos;
    vec3 color;
};

layout (binding = 2) uniform LightsUniform {
    vec3 direction;                 // 平行光
    vec3 color;                     // 平行光颜色
    PointLight point_lights[3];     // 点光源
} lights;


const float PI = 3.1415926535;
const float SPEC_ADD = 1.1;    // 补偿反射项

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
    return roughness2 / (PI * n * n);
}

// 几何遮挡函数
// G/4(NoV)(NoL)
float G2(float NoL, float NoV, float roughness2) {
    float lambda_in = NoV * sqrt(roughness2 + NoL * sqrt(NoL - roughness2 * NoL));
    float lambda_out = NoL * sqrt(roughness2 + NoV * sqrt(NoV - roughness2 * NoV));
    return 0.5 / (lambda_in + lambda_out);
}

vec3 compute_direction_light(vec3 direction, vec3 color, vec3 pos, vec3 N, vec3 V, vec3 F0, float roughness2) {
    vec3 H = normalize(V + direction);
    float NoL = max(dot(N, direction), 0);
    float NoV = max(dot(N, V), 0);
    float LoH = max(dot(direction, H), 0);
    vec3 diffuse_color = material.color * (1 - material.metallic) / PI;

    vec3 spec_color = F(F0, LoH) * G2(NoL, NoV, roughness2);

    return (diffuse_color + spec_color * SPEC_ADD) * NoL * color;
}

vec3 compute_point_light(vec3 light_pos, vec3 color, vec3 pos, vec3 N, vec3 V, vec3 F0, float roughness2) {
    vec3 L = normalize(light_pos - pos);
    float distance = length(light_pos - pos);
    return compute_direction_light(L, color, pos, N, V, F0, roughness2) / (distance * distance);
}

void main() {
    // 转换PBR参数
    float roughness = material.roughness * material.roughness;
    float reflectance = 0.16 * material.reflectance * material.reflectance;
    vec3 F0 = material.color * material.metallic + (reflectance * (1 - material.metallic));

    vec3 N = normalize(frag_normal);
    vec3 V = normalize(camera.pos - frag_pos);
    float roughness2 = clamp(roughness * roughness, 0.001, 0.999);

    vec3 color = compute_direction_light(-lights.direction, lights.color, frag_pos, N, V, F0, roughness2);
    for (int i = 0; i < 3; i ++) {
        color += compute_point_light(lights.point_lights[i].pos, lights.point_lights[i].color, frag_pos, N, V, F0, roughness2);
    }

    color = color / (color + 1);
    color = pow(color, vec3(1 / 2.2));

    out_color = vec4(color, 1);
}
