#version 450

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;

// InstanceInfo 实例信息
layout (location = 3) in vec3 offset;
layout (location = 4) in float scale;

layout (location = 0) out vec3 frag_pos;
layout (location = 1) out vec3 frag_normal;
layout (location = 2) out vec3 frag_color;

layout (binding = 0) uniform CameraUniform {
    vec3 pos;
    mat4 view;
    mat4 project;
} camera;

void main() {
    frag_pos = in_pos * scale + offset;
    frag_normal = in_normal;
    frag_color = in_color;
    gl_Position = camera.project * camera.view * vec4(frag_pos, 1);
}
