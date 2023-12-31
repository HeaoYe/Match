#version 450

layout(location = 0) in vec3 frag_pos;
layout(location = 1) in vec3 frag_normal;

layout(location = 0) out vec4 out_pos;
layout(location = 1) out vec4 out_normal;

void main() {
    // 最后一个Float存储深度信息
    out_pos = vec4(frag_pos, gl_FragCoord.z);
    // 若有Fragment，则将NormalBuffer的最后一个Float设为1
    out_normal = vec4(normalize(frag_normal), 1);
}
