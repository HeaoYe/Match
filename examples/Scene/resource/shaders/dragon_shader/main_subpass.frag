#version 450

layout(location = 0) in vec3 frag_pos;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec3 frag_color;

layout(location = 0) out vec4 out_pos;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_color;

void main() {
    out_pos = vec4(frag_pos, gl_FragCoord.z);
    out_normal = vec4(normalize(frag_normal), 1);
    out_color = vec4(frag_color, 1);
}
