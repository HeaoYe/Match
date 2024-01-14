#version 460

#extension GL_EXT_ray_tracing : require

struct RayInfo {
    int count;
    uint rnd_state;
    vec3 color;
    vec3 albedo;
};

layout (location = 0) rayPayloadInEXT RayInfo ray;

void main() {
    ray.color += ray.albedo * vec3(0.8, 0.8, 0.9);
}
