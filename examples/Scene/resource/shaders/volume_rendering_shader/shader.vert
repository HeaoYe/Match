#version 450

vec2 positions[3] = vec2[](
    vec2(1, 1),
    vec2(-3, 1),
    vec2(1, -3)
);

layout (location = 0) out vec2 ndc_pos;

void main() {
    ndc_pos = positions[gl_VertexIndex];
    gl_Position = vec4(ndc_pos, 0, 1);
}
