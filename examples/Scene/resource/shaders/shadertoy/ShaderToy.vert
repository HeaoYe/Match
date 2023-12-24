#version 450

layout (location = 0) in vec2 pos;

layout (push_constant) uniform Size {
    float width;
    float height;
} window_size;

layout (location = 0) out vec2 fragCoord;

void main() {
    gl_Position = vec4(pos, 0, 1);
    fragCoord = ((pos + 1) / 2) * vec2(window_size.width, window_size.height);
}
