#version 450

layout (location = 0) in vec3 frag_pos;
layout (location = 1) in vec3 frag_normal;
layout (location = 2) in vec3 frag_color;

layout (location = 0) out vec4 out_color;

const vec3 light_dir = normalize(vec3(0, -1, -0.3));

void main() {
    vec3 cool = vec3(0, 0, 0.55) + frag_color / 4;
    vec3 warm = vec3(0.3, 0.3, 0) + frag_color / 4;
    out_color = vec4(cool / 2 + warm * max(dot(-light_dir, normalize(frag_normal)), 0), 1);
}
