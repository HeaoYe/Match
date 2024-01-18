#version 450


layout (location = 0) in vec3 pos;

layout (binding = 0) uniform CameraUniform {
    vec3 pos;
    mat4 view;
    mat4 project;
} camera;

layout (location = 0) out vec3 frag_normal;
layout (location = 1) out vec4 frag_color;

void main() {
    frag_normal = vec3(pos);
    frag_color = vec4(1);
    gl_Position = camera.project * camera.view * vec4(pos / 400, 1);
}
