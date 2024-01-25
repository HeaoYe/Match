#version 450

layout (location = 0) in vec3 frag_normal;
layout (location = 1) in vec4 frag_color;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = vec4(frag_normal / 800 * 0.5, 1);
    // out_color = vec4(1);
}
