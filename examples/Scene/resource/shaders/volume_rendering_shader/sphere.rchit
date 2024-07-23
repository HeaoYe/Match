#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require

#include <MatchRandom>
#include "ray.glsl"
#include "perlin_noise.glsl"
#include "read_volume_data.glsl"

layout (binding = 3, scalar) buffer Sphere_ { MatchSphere spheres[]; } sphere_;
layout (binding = 5, scalar) buffer SphereType_ { int sphere_types[]; } sphere_type_;

layout (location = 0) rayPayloadInEXT RayInfo ray;

// 以VRE中的Tr项 为 pdf，采样t
float sample_t_from_Tr(float sigma_maj) {
    return - log(1 - rnd(ray.rnd_state)) / sigma_maj;
}

// 以phase_function为pdf，采样光线散射方向
vec3 sample_direction_from_phase_function(vec3 direction, float g) {
    float cos_theta;
    if (g == 0) {
        cos_theta = rnd(ray.rnd_state) * 2 - 1;
    } else {
        cos_theta = (1 + g * g - ((1 - g * g) / (1 + g - 2 * g * rnd(ray.rnd_state)))) / (- 2 * g);
    }
    float sin_theta = sqrt(1 - cos_theta * cos_theta);
    float phi = 2 * PI * rnd(ray.rnd_state);

    vec3 result = vec3(sin_theta * cos(phi), cos_theta, sin_theta * sin(phi));
    vec3 up = abs(direction.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 X = normalize(cross(up, direction));
    vec3 Z = normalize(cross(X, direction));
    return normalize(result.x * X + result.y * direction + result.z * Z);
}

// 使用 null scattering(零散射) 和 delta tracking(增量跟踪) 算法，评估VRE(体渲染方程)
void render_particular_medium(MatchSphere sphere, int sphere_type) {
    vec3 current_origin = gl_WorldRayOriginEXT;
    float sigma_maj = 2;
    float g = 0;
    if (sphere_type == 1) {
        sigma_maj = args.sigma_maj_uniform;
        g = args.g_uniform;
    } else if (sphere_type == 2) {
        sigma_maj = args.sigma_maj_perlin;
        g = args.g_perlin;
    } else if (sphere_type == 3) {
        sigma_maj = args.sigma_maj_cloud;
        g = args.g_cloud;
    } else if (sphere_type == 4) {
        sigma_maj = args.sigma_maj_smoke;
        g = args.g_smoke;
    }

    // 防止sigma_maj=0导致的除零错误
    if (sigma_maj < 1e-3) {
        sigma_maj = 1e-3;
    }

    while (true) {
        float t = sample_t_from_Tr(sigma_maj);
        vec3 sample_point = current_origin + gl_WorldRayDirectionEXT * t;

        // 判断采样点是否在参与介质内
        if (distance(sample_point, sphere.center) > sphere.radius) {
            // 不在参与介质内，结束体渲染
            return;
        } else {
            // 在参与介质内，评估VRE中的积分项

            float sigma_a = 0;  // 根据参与介质种类的不同，得到参与介质在采样点处的sigma_a
            float sigma_s = 0;  // 根据参与介质种类的不同，得到参与介质在采样点处的sigma_s

            if (sphere_type == 1) {
                sigma_a = args.sigma_a_uniform;
                sigma_s = args.sigma_s_uniform;
            } else if (sphere_type == 2) {
                float pn = perlin_fbm(sample_point) * 2 - 1;
                sigma_a = args.sigma_a_perlin * pn;
                sigma_s = args.sigma_s_perlin * pn;
            } else if (sphere_type == 3) {
                float volume_data = sample_volume_data(sample_point, sphere, sphere_type);
                sigma_a = args.sigma_a_cloud * volume_data;
                sigma_s = args.sigma_s_cloud * volume_data;
            } else if (sphere_type == 4) {
                float volume_data = sample_volume_data(sample_point, sphere, sphere_type);
                sigma_s = args.sigma_s_smoke * volume_data;

                // 减少边缘的吸收系数
                if (volume_data < 0.18) {
                    volume_data *= smoothstep(0.09, 0.18, volume_data);
                }
                sigma_a = args.sigma_a_smoke * volume_data;
            }

            float u = rnd(ray.rnd_state);
            if (u < sigma_a / sigma_maj) {
                // 发生吸收事件
                // 光线被吸收，不再传播
                ray.count += 1000;
                return;
            } else if (u < (sigma_a + sigma_s) / sigma_maj) {
                // 发生散射事件
                // 将光线散射，改变光线方向
                // 散射方向由参与介质的相函数决定
                ray.direction = sample_direction_from_phase_function(-ray.direction, g);
                // 光线负载的反射率需要 先乘相函数的值，再除概率密度函数的值
                // 因为给定出射方向，相函数在所有入射方向上的积分等于一，所以以相函数为pdf，采样方向， phase_function / pdf = 1，抵消了
                // ray.albedo *= phase_function / pdf;
                return;
            } else {
                // 发生零散射事件
                // 零散射事件不与光交互，从采样点继续传播
                current_origin = sample_point;
            }
        }
    }
}

void main() {
    MatchSphere sphere = sphere_.spheres[gl_PrimitiveID];
    int sphere_type = sphere_type_.sphere_types[gl_PrimitiveID];

    vec3 center = (gl_ObjectToWorldEXT * vec4(sphere.center, 1)).xyz;
    vec3 pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 normal = normalize(pos - center);

    ray.count += 1;
    ray.origin = pos;

    if (sphere_type == 0) {
        // 渲染大地
        ray.albedo *= vec3(0.7, 0.7, 1);

        float theta = 2 * PI * (rnd(ray.rnd_state) - 0.5);
        float phi = 2 * PI * rnd(ray.rnd_state);

        ray.direction = normalize(normal + vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)));
    } else {
        // 渲染参与介质
        render_particular_medium(sphere, sphere_type);
    }
}
