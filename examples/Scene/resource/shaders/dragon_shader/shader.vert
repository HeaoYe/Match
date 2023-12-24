#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;

layout(location = 3) in vec3 offset;

layout(binding = 0) uniform CameraUniform {
    vec3 pos;
    vec3 direction;
    mat4 view;
    mat4 project;
} camera;

struct PointLight {
    vec3 pos;
    vec3 color;
};

layout(binding = 1) uniform LightUniform {
    PointLight lights[10];
    uint num;
} light;

layout(location = 0) out vec3 frag_color;


void main() {
    vec3 pos = offset + in_pos;
    gl_Position = camera.project * camera.view * vec4(pos, 1);

    vec3 cool = vec3(0, 0, 0.55) + in_color / 4;
    vec3 warm = vec3(0.3, 0.3, 0) + in_color / 4;
    vec3 high_light = vec3(2, 2, 2);
    frag_color = cool / 2;
    for (uint i = 0; i < light.num; i ++) {
        vec3 l = light.lights[i].pos - pos;
        vec3 l_n = normalize(l);
        vec3 normal = normalize(in_normal);
        vec3 r = reflect(-l_n, normal);
        vec3 v = normalize(camera.pos - pos);
        float s = clamp(100 * dot(r, v) - 97, 0, 1);
        frag_color += clamp(dot(l_n, normal), 0, 1) * light.lights[i].color * mix(warm, high_light, s);
    }
}
