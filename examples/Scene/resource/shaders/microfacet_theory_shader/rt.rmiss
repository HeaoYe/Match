#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#include "ray.glsl"

layout (location = 0) rayPayloadInEXT RayInfo ray;

const vec3 sky_light = vec3(0.9, 1, 1);
const vec3 sun_light = vec3(50);
const vec3 sun_direction = normalize(vec3(0.4, 1, 0.5));

void main() {
    ray.end = true;

    if (dot(ray.direction, sun_direction) > 0.997) {
        ray.color += ray.albedo * sun_light;
    } else {
        ray.color += ray.albedo * sky_light;
    }
}
