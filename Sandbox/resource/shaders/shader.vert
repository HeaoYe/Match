#version 450

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 offset;  // 每个实例传入一份offset数据
layout(binding = 0) uniform PosScaler {
    float x_pos_scale;
    float y_pos_scale;
} pos_ubo;
layout(binding = 1) uniform ColorScaler {
    float color_scale;
} color_ubo;

layout(location = 0) out vec3 frag_color;

void main() {
    vec2 vertex_pos = vec2(pos_ubo.x_pos_scale * in_pos.x, pos_ubo.y_pos_scale * in_pos.y);
    gl_Position = vec4(offset + vertex_pos, 0, 1);
    frag_color = color_ubo.color_scale * in_color;
}
