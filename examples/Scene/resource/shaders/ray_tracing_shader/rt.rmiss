#version 460

#extension GL_EXT_ray_tracing : require

layout (location = 0) rayPayloadInEXT vec3 ray_color;

layout (push_constant) uniform Constants {
    vec3 sky_color;
    vec3 pos;
    vec3 color;
    float intensity;
    int sample_count;
} constant;

void main() {
    // 若未命中,设置颜色为sky_color
    ray_color = constant.sky_color;
}
