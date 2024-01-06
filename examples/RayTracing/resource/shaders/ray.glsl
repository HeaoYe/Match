#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

struct hitPayload {
    vec3 hit_value;
};

struct InstanceInfo {
    uint64_t vertex_buffer_address;
    uint64_t index_buffer_address;
};

struct Vertex {
    vec3 pos;
    vec3 normal;
    vec3 color;
};
