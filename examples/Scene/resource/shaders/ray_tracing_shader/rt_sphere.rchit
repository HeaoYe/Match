#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require

struct MyCustomData1 {
    vec3 color;
};

layout (binding = 7, scalar) buffer MyCustomData1_ { MyCustomData1 datas[]; } data_;

layout (location = 0) rayPayloadInEXT vec3 ray_color;

void main() {
    MyCustomData1 data = data_.datas[gl_PrimitiveID];
    ray_color = data.color;
}
