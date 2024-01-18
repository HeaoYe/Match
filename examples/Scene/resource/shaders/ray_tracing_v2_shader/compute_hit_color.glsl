#extension GL_EXT_ray_tracing : require

#include <MatchRandom>

struct Material {
    vec3 albedo;
    float roughness;
    vec3 spec_albedo;
    float spec_prob;
    vec3 light_color;
    float light_intensity;
};

struct RayInfo {
    int count;
    uint rnd_state;
    vec3 color;
    vec3 albedo;
    vec3 origin;
    vec3 direction;
};

layout (binding = 1) uniform accelerationStructureEXT instance;

layout (location = 0) rayPayloadInEXT RayInfo ray;

// 高斯分布随机数
float uniform_rnd(inout uint state, float mean, float std) {
    return mean + std * (sqrt(-2 * log(rnd(state))) * cos(2 * 3.1415926535 * rnd(state)));
}

void compute_hit_color(vec3 pos, vec3 normal, Material material) {
    ray.count += 1;

    float is_spec = float(material.spec_prob >= rnd(ray.rnd_state));

    ray.color += ray.albedo * material.light_color * material.light_intensity;
    ray.albedo *= mix(material.albedo, material.spec_albedo, is_spec);

    vec3 random_direction = normalize(vec3(uniform_rnd(ray.rnd_state, 0, 1), uniform_rnd(ray.rnd_state, 0, 1), uniform_rnd(ray.rnd_state, 0, 1)));
    vec3 diff_direction = normalize(normal + random_direction);
    ray.direction = normalize(mix(
        reflect(gl_WorldRayDirectionEXT, normal),
        diff_direction,
        material.roughness * material.roughness * (1 - is_spec)
    ));
    ray.origin = pos;
}
