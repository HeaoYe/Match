#version 450

layout (location = 0) in vec3 frag_pos;
layout (location = 1) in vec3 frag_normal;
layout (location = 2) in vec3 frag_color;

layout (location = 0) out vec4 out_color;

layout (binding = 2) uniform Light_ {
    vec3 pos;
    vec3 color;
    float intensity;
} light;

void main() {
    vec3 L = normalize(light.pos - frag_pos);
    float L_D = length(light.pos - frag_pos);
    vec3 normal = normalize(frag_normal);

    out_color = vec4(vec3(0.05) + max(dot(L, normal), 0) * light.color * light.intensity * 0.8 / L_D / L_D, 1);
}
