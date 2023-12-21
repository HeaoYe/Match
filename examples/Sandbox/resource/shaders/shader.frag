#version 450

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_uv;  // 纹理坐标

layout(binding = 2) uniform sampler2D texture_sampler; // 纹理采样器

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(texture_sampler, frag_uv);
}
