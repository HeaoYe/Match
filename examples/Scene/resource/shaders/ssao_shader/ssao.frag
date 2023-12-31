#version 450

layout (binding = 0) uniform sampler2D pos_buffer;
layout (binding = 1, input_attachment_index = 1) uniform subpassInput normal_buffer;

layout (location = 0) out vec4 out_ssao_value;

layout (binding = 2) uniform SSAOSamples {
    vec4 samples[64];
} ssao;

layout (binding = 3) uniform sampler2D RandomVecs;

layout (binding = 4) uniform CameraUniform {
    vec3 pos;
    mat4 view;
    mat4 project;
} camera;

layout (push_constant) uniform Constant {
    vec2 window_size;
    int a;
    int sample_count;
    float r;
} constant;

void main() {
    vec4 normal_s = subpassLoad(normal_buffer);
    float s = normal_s.w;
    if (s == 0) {
        out_ssao_value = vec4(-1);
        return;
    }
    vec3 normal = normal_s.xyz;
    vec3 pos = texture(pos_buffer, gl_FragCoord.xy / constant.window_size).xyz;

    vec3 random_vec = texture(RandomVecs, gl_FragCoord.xy / float(constant.a)).xyz;
    vec3 T = normalize(random_vec - dot(random_vec, normal) * normal);
    vec3 B = cross(normal, T);
    mat3 TBN = mat3(T, B, normal);

    float ssao_value = 0;

    for (uint i = 0; i < constant.sample_count; i ++) {
        vec3 sample_point = pos + constant.r * (TBN * ssao.samples[i].xyz);
        vec4 cliped_point = camera.project * camera.view * vec4(sample_point, 1);
        cliped_point.xyz /= cliped_point.w;
        // 从透视矩阵变换后的[-1, 1]转为[0, 1]
        cliped_point.xyz = cliped_point.xyz * 0.5 + 0.5;
        // 对于屏幕 左下角的坐标为(0, 0)
        // 对于纹理 左上角的坐标为(0, 0)
        // 转换cliped_point.xy作为纹理坐标
        cliped_point.y = 1 - cliped_point.y;
        // 获取原始depth
        float depth = texture(pos_buffer, cliped_point.xy).w;
        // 若原始depth比sample_point的depth小，ssao_value+1
        if (depth < cliped_point.z) {
            ssao_value += 1;
        }
    }
    ssao_value = 1 - (ssao_value / float(constant.sample_count));
    out_ssao_value = vec4(ssao_value);
}
