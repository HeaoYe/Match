#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include <MatchTypes>

layout (location = 1) rayPayloadInEXT bool is_shadow;

void main() {
    is_shadow = false;
}
