#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_uv;   // 纹理坐标
layout(location = 3) in vec3 offset;  // 每个实例传入一份offset数据
layout(binding = 0) uniform PosScaler {
    float x_pos_scale;
    float y_pos_scale;
} pos_ubo;
layout(binding = 1) uniform ColorScaler {
    float color_scale;
} color_ubo;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_uv;

void main() {
    vec2 vertex_pos = vec2(pos_ubo.x_pos_scale * in_pos.x, pos_ubo.y_pos_scale * in_pos.y);
    vec3 pos = offset + vec3(vertex_pos, in_pos.z);
    // 将深度限制在1以内
    pos.z /= 20;
    gl_Position = vec4(pos, 1);
    frag_color = color_ubo.color_scale * in_color;
    // 将in_uv从[0, 1]变换到[-2, 2]
    // frag_uv = (in_uv - 0.5) * 4;
    frag_uv = in_uv * 0.98 + 0.01;
}
