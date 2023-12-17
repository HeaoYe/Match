#version 450

layout(location = 0) out vec3 frag_color;

vec2 positions[3] = vec2[](
    vec2(0, 0.5),
    vec2(0.5, -0.5),
    vec2(-0.5, -0.5)
);

vec3 colors[3] = vec3[](
    vec3(1, 0, 0.2),
    vec3(0.5, 1, 0),
    vec3(0, 0.6, 1)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0, 1);
    frag_color = colors[gl_VertexIndex];
}
