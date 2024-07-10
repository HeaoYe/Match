#version 460

#extension GL_EXT_ray_tracing : require

struct RayInfo {
    int count;
    uint rnd_state;
    vec3 color;
    vec3 albedo;
    vec3 origin;
    vec3 direction;
};

layout (location = 0) rayPayloadInEXT RayInfo ray;

const vec3 sky_light = vec3(0.9, 1, 1);
const vec3 sun_direction = normalize(vec3(0.4, 1, 0.5));

void main() {
    ray.count += 1000;
    ray.color += ray.albedo * sky_light;
}
