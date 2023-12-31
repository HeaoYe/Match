#version 450

layout (binding = 0, input_attachment_index = 0) uniform subpassInput pos_buffer;
layout (binding = 1, input_attachment_index = 1) uniform subpassInput normal_buffer;
layout (binding = 2) uniform sampler2D ssao_buffer;

layout (location = 0) out vec4 out_color;

layout (push_constant) uniform Constants {
    vec2 window_size;
    int ssao_enable;
    int blur_kernel_size;
} constant;

const vec3 light_dir = vec3(0, -1, -0.3);
const vec3 light_color = vec3(1, 0.6 ,0);

void main() {
    vec4 normal_s = subpassLoad(normal_buffer);
    float s = normal_s.w;
    if (s == 0) {
        out_color = vec4(0.3, 0.7, 0.4, 1);
        return;
    }
    vec3 normal = normal_s.xyz;
    vec3 pos = subpassLoad(pos_buffer).xyz;

    vec2 uv_per_pixel = 1 / constant.window_size;
    vec2 uv = gl_FragCoord.xy * uv_per_pixel;

    float left = float(-constant.blur_kernel_size) / 2.0;

    float ssao_value = 0;
    int count = 0;
    for (int x = 0; x < constant.blur_kernel_size; x ++) {
        for (int y = 0; y < constant.blur_kernel_size; y ++) {
            float current_ssao_value = texture(ssao_buffer, uv + vec2(x + left, y + left) * uv_per_pixel).x;
            if (current_ssao_value >= 0) {
                ssao_value += current_ssao_value;
                count += 1;
            }
        }
    }
    ssao_value = ssao_value / float(count);

    if (constant.ssao_enable == 0) {
        ssao_value = 1;
    }
    vec3 color = ssao_value * vec3(0.3);
    out_color = vec4(color + max(dot(-light_dir, normal), 0) * light_color, 1);
}
