#version 450

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_uv;  // 纹理坐标


// 设置第几个Set的第几个binding，默认第0个Set
layout(set = 0, binding = 2) uniform sampler2D texture_sampler; // 纹理采样器

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(texture_sampler, frag_uv);
}
