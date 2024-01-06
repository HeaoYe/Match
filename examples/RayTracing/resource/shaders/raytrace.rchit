#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

#include "ray.glsl"

layout (location = 0) rayPayloadInEXT hitPayload prd;
hitAttributeEXT vec3 attribs;

void main() {
  prd.hit_value = vec3(1.0, 0.5, 1.0);
}
