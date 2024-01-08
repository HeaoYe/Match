#version 450

layout (location = 0) in vec2 ndc_pos;

layout (location = 0) out vec4 out_color;

layout (binding = 0) uniform sampler2D ray_tracing_result_image;

void main () {
    vec2 uv = ndc_pos * 0.5 + 0.5;
    uv.y = 1 - uv.y;

    out_color = texture(ray_tracing_result_image, uv);
}
