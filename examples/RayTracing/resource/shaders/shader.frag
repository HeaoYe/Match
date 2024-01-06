#version 450

layout(binding = 0) uniform sampler2D image;

layout (binding = 0, set = 1) uniform G {
    vec2 window_size;
    vec3 clear_color;
} g;

layout(binding = 1, set = 1) uniform CameraUniform {
    vec3 pos;
    mat4 view;
    mat4 project;
} camera;

layout (location = 0) out vec4 out_color;

void main() {
    vec2 uv = gl_FragCoord.xy / g.window_size;
    uv.y = 1 - uv.y;
    out_color = texture(image, uv);
}
