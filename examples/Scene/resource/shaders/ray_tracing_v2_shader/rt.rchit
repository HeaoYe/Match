#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require

#include <MatchTypes>
#include <MatchRandom>

struct SphereMaterial {
    vec3 albedo;
    float roughness;
    vec3 light_color;
    float light_intensity;
};

struct RayInfo {
    int count;
    uint rnd_state;
    vec3 color;
    vec3 albedo;
};

layout (binding = 1) uniform accelerationStructureEXT instance;
layout (binding = 3, scalar) buffer Sphere_ { MatchSphere spheres[]; } sphere_;
layout (binding = 4, scalar) buffer SphereMaterial_ { SphereMaterial materials[]; } material_;

layout (location = 0) rayPayloadInEXT RayInfo ray;

const int max_ray_recursion_depth = 16;

void main() {
    MatchSphere sphere = sphere_.spheres[gl_PrimitiveID];
    SphereMaterial material = material_.materials[gl_PrimitiveID];

    vec3 center = (gl_ObjectToWorldEXT * vec4(sphere.center, 1)).xyz;
    vec3 pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 normal = normalize(pos - center);

    ray.count += 1;
    ray.color += ray.albedo * material.light_color * material.light_intensity;
    ray.albedo *= material.albedo;

    if (ray.count >= max_ray_recursion_depth) {
        return;
    }

    vec3 random_direction = normalize(vec3(rnd(ray.rnd_state), rnd(ray.rnd_state), rnd(ray.rnd_state)));
    vec3 direction = normalize(reflect(gl_WorldRayDirectionEXT, normal) + material.roughness * random_direction);
    traceRayEXT(
        instance,
        gl_RayFlagsOpaqueEXT,
        0xff,
        0, 0,
        0,
        pos,
        0.001,
        direction,
        10000.0,
        0
    );
}
