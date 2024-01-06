#version 460
#extension GL_EXT_ray_tracing : require

struct hitPayload {
  vec3 hit_value;
};

layout (location = 0) rayPayloadInEXT hitPayload prd;

void main() {
    prd.hit_value = vec3(0.0, 0.1, 0.3);
}
