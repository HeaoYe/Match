#include <MatchTypes>
#include <MatchVolume>

layout (binding = 7) readonly buffer VolumeBufferCloud {
    float volume_data_cloud[volume_data_size];
};

layout (binding = 8) readonly buffer VolumeBufferSmoke {
    float volume_data_smoke[volume_data_size];
};

float volume_data_at(ivec3 idx, int sphere_type) {
    int index = pos_to_volume_data_idx(idx);
    if (index < 0) {
        return 0;
    }
    if (sphere_type == 3) {
        return volume_data_cloud[index];
    }
    return volume_data_smoke[index];
}

// 采样体积数据
float sample_volume_data(vec3 pos, MatchSphere sphere, int sphere_type) {
    pos -= sphere.center;
    pos /= 2.f * sphere.radius / float(sqrt(3.));
    pos += 0.5;
    pos *= volume_data_resolution;

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
