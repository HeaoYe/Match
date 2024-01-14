#version 460

#extension GL_EXT_ray_tracing : require

layout (location = 1) rayPayloadInEXT bool is_in_shadow;

void main() {
    // 若未命中,设置is_in_shadow为false
    is_in_shadow = false;
}
