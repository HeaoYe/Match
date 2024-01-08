#version 450

vec2 positions[4] = vec2[](
    vec2(1, 1),
    vec2(-1, 1),
    vec2(1, -1),
    vec2(-1, -1)
);

layout (location = 0) out vec2 ndc_pos;

void main() {
    ndc_pos = positions[gl_VertexIndex + gl_InstanceIndex];
    gl_Position = vec4(ndc_pos, 0, 1);
}
