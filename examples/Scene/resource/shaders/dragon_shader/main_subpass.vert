#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;

layout(location = 3) in vec3 offset;

layout(binding = 0) uniform CameraUniform {
    vec3 pos;
    mat4 view;
    mat4 project;
} camera;

layout(location = 0) out vec3 frag_pos;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec3 frag_color;

void main() {
    frag_pos = offset + in_pos;
    gl_Position = camera.project * camera.view * vec4(frag_pos, 1);
    frag_normal = in_normal;
    frag_color = in_color;
}
