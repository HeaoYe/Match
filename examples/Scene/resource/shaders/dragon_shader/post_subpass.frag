#version 450


layout(binding = 0, input_attachment_index = 0) uniform subpassInput buffers[3];

layout(location = 0) out vec4 out_color;

struct PointLight {
    vec3 pos;
    vec3 color;
};

layout(binding = 1) uniform CameraUniform {
    vec3 pos;
    mat4 view;
    mat4 project;
} camera;

layout(binding = 2) uniform LightUniform {
    PointLight lights[10];
    uint num;
} light;


void main() {
    if (subpassLoad(buffers[0]).w == 1) {
        out_color = vec4(1, 1, 1, 0);
        return;
    }
    // pos
    // out_color = vec4(subpassLoad(buffers[0]).xyz, 1);

    // z-buffer
    // out_color = vec4(vec3(subpassLoad(buffers[0]).w), 1);

    // normal
    // out_color = vec4((subpassLoad(buffers[1]).xyz + 1) / 2, 1);

    // color
    // out_color = vec4(subpassLoad(buffers[2]).rgb, 1);
    // return;
    vec3 pos = subpassLoad(buffers[0]).xyz;
    vec3 in_normal = subpassLoad(buffers[1]).xyz;
    vec3 in_color = subpassLoad(buffers[2]).rgb;

    vec3 cool = vec3(0, 0, 0.55) + in_color / 4;
    vec3 warm = vec3(0.3, 0.3, 0) + in_color / 4;
    vec3 high_light = vec3(2, 2, 2);
    vec3 color = cool / 2;
    for (uint i = 0; i < light.num; i ++) {
        vec3 l = light.lights[i].pos - pos;
        vec3 l_n = normalize(l);
        vec3 normal = normalize(in_normal);
        vec3 r = reflect(-l_n, normal);
        vec3 v = normalize(camera.pos - pos);
        float s = clamp(100 * dot(r, v) - 97, 0, 1);
        color += clamp(dot(l_n, normal), 0, 1) * light.lights[i].color * mix(warm, high_light, s);
    }

    out_color = vec4(color, 1);
}
