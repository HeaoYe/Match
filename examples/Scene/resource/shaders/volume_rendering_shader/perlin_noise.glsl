#include "args.glsl"

// 从0到255，一共256个数字，打乱重排，组成的数组
// 用于确定每个晶格顶点的随机向量
const int random_list[256] = {
    151, 160, 137, 91, 90, 15,
	131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
	190,  6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
	88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168,  68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
	77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
	102, 143, 54,  65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208,  89, 18, 169, 200, 196,
	135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186,  3, 64, 52, 217, 226, 250, 124, 123,
	5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
	223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172, 9,
	129, 22, 39, 253,  19, 98, 108, 110, 79, 113, 224, 232, 178, 185,  112, 104, 218, 246, 97, 228,
	251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,  81, 51, 145, 235, 249, 14, 239, 107,
	49, 192, 214,  31, 181, 199, 106, 157, 184,  84, 204, 176, 115, 121, 50, 45, 127,  4, 150, 254,
	138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};

// 柏林噪音用到的平滑函数
vec3 perlin_smooth(vec3 value) {
    return value * value * value * (value * (value * 6 - 15) + 10);
}

// 生成每个晶格顶点的随机向量的hash
int perlin_vec_hash(ivec3 grid) {
    return random_list[(random_list[(random_list[grid.x % 256] + grid.y) % 256] + grid.z) % 256];
}

// 任意向量 与 每个晶格顶点的随机向量 点乘
float perlin_vec_dot(vec3 vec, int vec_hash) {
    vec_hash = vec_hash % 8;
    if (vec_hash == 0) {
        return - vec.x - vec.y - vec.z;
    } else if (vec_hash == 1) {
        return + vec.x - vec.y - vec.z;
    } else if (vec_hash == 2) {
        return - vec.x + vec.y - vec.z;
    } else if (vec_hash == 3) {
        return + vec.x + vec.y - vec.z;
    } else if (vec_hash == 4) {
        return - vec.x - vec.y + vec.z;
    } else if (vec_hash == 5) {
        return + vec.x - vec.y + vec.z;
    } else if (vec_hash == 6) {
        return - vec.x + vec.y + vec.z;
    } else if (vec_hash == 7) {
        return + vec.x + vec.y + vec.z;
    }
}

float perlin_noise(vec3 pos) {
    ivec3 grid = ivec3(floor(pos));
    vec3 rel_pos = fract(pos);

    // 采样晶格八个顶点
    float v000 = perlin_vec_dot(rel_pos - vec3(0, 0, 0), perlin_vec_hash(grid + ivec3(0, 0, 0)));
    float v100 = perlin_vec_dot(rel_pos - vec3(1, 0, 0), perlin_vec_hash(grid + ivec3(1, 0, 0)));
    float v010 = perlin_vec_dot(rel_pos - vec3(0, 1, 0), perlin_vec_hash(grid + ivec3(0, 1, 0)));
    float v110 = perlin_vec_dot(rel_pos - vec3(1, 1, 0), perlin_vec_hash(grid + ivec3(1, 1, 0)));
    float v001 = perlin_vec_dot(rel_pos - vec3(0, 0, 1), perlin_vec_hash(grid + ivec3(0, 0, 1)));
    float v101 = perlin_vec_dot(rel_pos - vec3(1, 0, 1), perlin_vec_hash(grid + ivec3(1, 0, 1)));
    float v011 = perlin_vec_dot(rel_pos - vec3(0, 1, 1), perlin_vec_hash(grid + ivec3(0, 1, 1)));
    float v111 = perlin_vec_dot(rel_pos - vec3(1, 1, 1), perlin_vec_hash(grid + ivec3(1, 1, 1)));

    // 三线性插值
    vec3 smooth_value = perlin_smooth(rel_pos);
    return clamp(
        mix(
            mix(
                mix(v000, v100, smooth_value.x),
                mix(v010, v110, smooth_value.x),
                smooth_value.y
            ),
            mix(
                mix(v001, v101, smooth_value.x),
                mix(v011, v111, smooth_value.x),
                smooth_value.y
            ),
            smooth_value.z
        ),
        -1, 1
    );
}

float perlin_fbm(vec3 pos) {
    pos *= args.freq;
    float result = 0;
    for (int i = 0; i < args.OCT; i ++) {
        result += perlin_noise(pos) * pow(args.L, - args.H * i);
        pos *= args.L;
    }
    return clamp(result * 2, 0, 4);
}
