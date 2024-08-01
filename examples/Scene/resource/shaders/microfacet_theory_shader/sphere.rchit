#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require

#include <MatchTypes>
#include "compute_hit_color.glsl"

layout (binding = 3, scalar) buffer Sphere_ { MatchSphere spheres[]; } sphere_;
layout (binding = 4, scalar) buffer Material_ { Material materials[]; } material_;

void main() {
    MatchSphere sphere = sphere_.spheres[gl_PrimitiveID];
    Material material = material_.materials[gl_PrimitiveID];

    vec3 center = (gl_ObjectToWorldEXT * vec4(sphere.center, 1)).xyz;
    vec3 pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 normal = normalize(pos - center);

    if (sphere.radius > 100) {
        float r = sqrt(rnd(ray.rnd_state));
        float theta = 2 * PI * rnd(ray.rnd_state);
        vec3 L_local = vec3(r * cos(theta), sqrt(1 - r * r), r * sin(theta));

        vec3 up = abs(normal.y) < 0.99999 ? vec3(0, 1, 0) : vec3(0, 0, 1);
        vec3 X = normalize(cross(up, normal));
        vec3 Z = normalize(cross(X, normal));

        ray.albedo *= material.albedo;
        ray.origin = pos;
        ray.direction = normalize(L_local.x * X + L_local.y * normal + L_local.z * Z);
    } else {
        compute_hit_color(pos, normal, material);
    }
}
