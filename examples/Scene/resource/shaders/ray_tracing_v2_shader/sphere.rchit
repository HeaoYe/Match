#version 460

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

    compute_hit_color(pos, normal, material);
}
