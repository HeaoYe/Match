#include <MatchTypes>

layout (binding = 7) readonly buffer VolumeBufferCloud {
    float volume_data_cloud[1000 * 1000 * 1000];
};

layout (binding = 8) readonly buffer VolumeBufferSmoke {
    float volume_data_smoke[1000 * 1000 * 1000];
};

float volume_data_at(ivec3 idx, int sphere_type) {
    if (idx.x >= 1000 || idx.y >= 1000 || idx.z >= 1000) {
        return 0;
    }
    if (idx.x < 0 || idx.y < 0 || idx.z < 0) {
        return 0;
    }
    if (sphere_type == 3) {
        return volume_data_cloud[idx.x * 1000 * 1000 + idx.y * 1000 + idx.z];
    }
    return volume_data_smoke[idx.x * 1000 * 1000 + idx.y * 1000 + idx.z];
}

// 采样体积数据
float sample_volume_data(vec3 pos, MatchSphere sphere, int sphere_type) {
    pos -= sphere.center;
    pos /= 2.f * sphere.radius / float(sqrt(3.));
    pos += 0.5;
    pos *= 1000;

    ivec3 grid = ivec3(floor(pos));
    vec3 rel_pos = fract(pos);

    float v100 = volume_data_at(grid + ivec3(1, 0, 0), sphere_type);
    float v000 = volume_data_at(grid + ivec3(0, 0, 0), sphere_type);
    float v010 = volume_data_at(grid + ivec3(0, 1, 0), sphere_type);
    float v110 = volume_data_at(grid + ivec3(1, 1, 0), sphere_type);
    float v001 = volume_data_at(grid + ivec3(0, 0, 1), sphere_type);
    float v101 = volume_data_at(grid + ivec3(1, 0, 1), sphere_type);
    float v011 = volume_data_at(grid + ivec3(0, 1, 1), sphere_type);
    float v111 = volume_data_at(grid + ivec3(1, 1, 1), sphere_type);

    return mix(
        mix(
            mix(v000, v100, rel_pos.x),
            mix(v010, v110, rel_pos.x),
            rel_pos.y
        ),
        mix(
            mix(v001, v101, rel_pos.x),
            mix(v011, v111, rel_pos.x),
            rel_pos.y
        ),
        rel_pos.z
    );
}
