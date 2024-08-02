#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#include "ray.glsl"
#include <MatchRandom>

struct Material {
    vec3 albedo;
    int type;

    float IOR;
    float K;

    float alpha_x;
    float alpha_y;
};

layout (binding = 1) uniform accelerationStructureEXT instance;

layout (location = 0) rayPayloadInEXT RayInfo ray;

const float PI = 3.1415926535;

// 计算折射方向，返回vec4(折射方向, 相对折射率)
vec4 Refract(vec3 V_local, vec3 N_local, float cos_theta_i, float IOR) {
    // 处理V_local和N_local不在同一半球的情况
    if (cos_theta_i < 0) {
        IOR = 1 / IOR;
        cos_theta_i = -cos_theta_i;
        N_local = -N_local;
    }

    float sin2_theta_i = 1 - cos_theta_i * cos_theta_i;
    float sin2_theta_t = sin2_theta_i / (IOR * IOR);

    // 处理全反射
    if (sin2_theta_t >= 1) {
        return vec4(0);
    }

    // 计算折射方向
    float cos_theta_t = sqrt(1 - sin2_theta_t);
    return vec4(
        -V_local / IOR + (cos_theta_i / IOR - cos_theta_t) * N_local,
        IOR
    );
}

// 介电质菲涅尔公式，折射率为实数，返回光线被反射的比例
float Fresnel(float cos_theta_i, float IOR) {
    // 处理V_local和N_local不在同一半球的情况
    if (cos_theta_i < 0) {
        IOR = 1 / IOR;
        cos_theta_i = -cos_theta_i;
    }

    float sin2_theta_i = 1 - cos_theta_i * cos_theta_i;
    float sin2_theta_t = sin2_theta_i / (IOR * IOR);

    // 处理全反射
    if (sin2_theta_t >= 1) {
        return 1;
    }

    // 计算公式值
    float cos_theta_t = sqrt(1 - sin2_theta_t);
    vec2 denom = vec2(
        IOR * cos_theta_i + cos_theta_t,
        cos_theta_i + IOR * cos_theta_t
    );
    vec2 r;
    if (denom.x == 0) {
        r.x = 0;
        denom.x = 1;
    } else {
        r.x = IOR * cos_theta_i - cos_theta_t;
    }
    if (denom.y == 0) {
        r.y = 0;
        denom.y = 1;
    } else {
        r.y = cos_theta_i - IOR * cos_theta_t;
    }
    r /= denom;
    return 0.5 * dot(r, r);
}

// 复数乘法
vec2 ComplexMul(vec2 ab, vec2 cd) {
    return vec2(
        ab.x * cd.x - ab.y * cd.y,
        ab.x * cd.y + ab.y * cd.x
    );
}

// 复数除法
vec2 ComplexDiv(vec2 ab, vec2 cd) {
    return vec2(
        dot(ab, cd),
        ab.y * cd.x - ab.x * cd.y
    ) / dot(cd, cd);
}

// 复数开根
vec2 ComplexSqrt(vec2 ab) {
    float l = length(ab);
    return vec2(
        sqrt((l + ab.x) * 0.5),
        sqrt((l - ab.x) * 0.5)
    );
}

// 导体菲涅尔公式，折射率为复数，返回光线被反射的比例
float ComplexFresnel(float cos_theta_i, float IOR, float K) {
    // 用vec2表示复数，两个分量分别是实部和虚部
    vec2 complex_ior = vec2(IOR, K);
    vec2 sin2_theta_i = vec2(1 - cos_theta_i * cos_theta_i, 0);
    vec2 sin2_theta_t = ComplexDiv(sin2_theta_i, ComplexMul(complex_ior, complex_ior));

    // 计算公式值
    vec2 cos_theta_t = ComplexSqrt(vec2(1, 0) - sin2_theta_t);
    vec2 r1 = ComplexDiv((complex_ior * cos_theta_i - cos_theta_t), (complex_ior * cos_theta_i + cos_theta_t));
    vec2 r2 = ComplexDiv((cos_theta_i - ComplexMul(complex_ior, cos_theta_t)), cos_theta_i + ComplexMul(complex_ior, cos_theta_t));
    return 0.5 * (length(r1) + length(r2));
}

// 获取局部坐标系某方向的 tan2_theta, cos_phi, sin_phi
vec3 get_tan2_theta_cos_sin_phi(vec3 direction_local, float cos2_theta) {
    float r = sqrt(1 - cos2_theta);
    if (r == 0) {
        return vec3(0, 1, 0);
    } else {
        return vec3(
            (1 - cos2_theta) / cos2_theta,
            direction_local.x / r,
            direction_local.z / r
        );
    }
}

// 微表面法线分布函数
float D(vec3 N_micro_local, float alpha_x, float alpha_y) {
    float cos2_theta = N_micro_local.y * N_micro_local.y;
    if (cos2_theta == 0) {
        return 0;
    }
    vec3 tcs = get_tan2_theta_cos_sin_phi(N_micro_local, cos2_theta);
    float e = tcs.x * (((tcs.y * tcs.y) / (alpha_x * alpha_x)) + ((tcs.z * tcs.z) / (alpha_y * alpha_y)));
    return 1 / (PI * alpha_x * alpha_y * cos2_theta * cos2_theta * (1 + e) * (1 + e));
}

// Lambda函数，用于辅助计算 掩蔽函数 和 掩蔽-阴影函数
float Lambda(vec3 direction_local, float alpha_x, float alpha_y) {
    float cos2_theta = direction_local.y * direction_local.y;
    if (cos2_theta == 0) {
        return 0;
    }
    vec3 tcs = get_tan2_theta_cos_sin_phi(direction_local, cos2_theta);
    float alpha2 = alpha_x * alpha_x * tcs.y * tcs.y + alpha_y * alpha_y * tcs.z * tcs.z;
    return (sqrt(1 + tcs.x * alpha2) - 1) * 0.5;
}

// 掩蔽函数
float G1(vec3 direction_local, float alpha_x, float alpha_y) {
    return 1 / (1 + Lambda(direction_local, alpha_x, alpha_y));
}

// 掩蔽-阴影函数
float G2(vec3 V_local, vec3 L_local, float alpha_x, float alpha_y) {
    return 1 / (1 + Lambda(V_local, alpha_x, alpha_y) + Lambda(L_local, alpha_x, alpha_y));
}

// 微表面可见法线概率密度函数
float DPDF(vec3 V_local, vec3 N_micro_local, float alpha_x, float alpha_y) {
    return G1(V_local, alpha_x, alpha_y) * D(N_micro_local, alpha_x, alpha_y) * abs(dot(V_local, N_micro_local) / V_local.y);
}

// 根据DPDF函数，重要性采样微表面可见法线
vec3 SampleVisibleNormal(vec3 V_local, float alpha_x, float alpha_y) {
    vec3 V_local_normalized = normalize(vec3(alpha_x * V_local.x, V_local.y, alpha_y * V_local.z));
    if (V_local_normalized.y < 0) {
        V_local_normalized = -V_local_normalized;
    }

    float r = sqrt(rnd(ray.rnd_state));
    float theta = 2 * PI * rnd(ray.rnd_state);
    vec2 uniform_sampled_circle = r * vec2(cos(theta), sin(theta));

    float h = sqrt(1 - uniform_sampled_circle.x * uniform_sampled_circle.x);
    uniform_sampled_circle.y = mix(h, uniform_sampled_circle.y, (1 + V_local_normalized.y) * 0.5);
    float y = sqrt(max(0, 1 - dot(uniform_sampled_circle, uniform_sampled_circle)));

    vec3 up = abs(V_local_normalized.y) < 0.99999 ? vec3(0, 1, 0) : vec3(0, 0, 1);
    vec3 X = normalize(cross(up, V_local_normalized));
    vec3 Z = normalize(cross(X, V_local_normalized));

    vec3 N_micro_local_normalized = uniform_sampled_circle.x * X + y * V_local_normalized + uniform_sampled_circle.y * Z;
    return normalize(vec3(alpha_x * N_micro_local_normalized.x, max(1e-6, N_micro_local_normalized.y), alpha_y * N_micro_local_normalized.z));
}

void compute_hit_color(vec3 pos, vec3 normal, Material material) {
    vec3 up = abs(normal.y) < 0.99999 ? vec3(0, 1, 0) : vec3(0, 0, 1);
    vec3 X = normalize(cross(up, normal));
    vec3 Z = normalize(cross(X, normal));
    // 计算局部坐标系观察方向
    vec3 V_local = normalize(vec3(
        dot(X, -ray.direction),
        dot(normal, -ray.direction),
        dot(Z, -ray.direction)
    ));
    // 局部坐标系光线方向
    vec3 L_local;
    // 双向散射分布函数值
    float bsdf = 1;
    // 概率密度函数值
    float pdf = 1;

    // 重要性采样微表面可见法线方向
    vec3 N_micro_local = vec3(0, 1, 0);
    float alpha_x = max(1e-3, material.alpha_x);
    float alpha_y = max(1e-3, material.alpha_y);
    bool is_smooth = max(material.alpha_x, material.alpha_y) < 1e-3 || material.IOR == 1;
    if (!is_smooth) {
        N_micro_local = SampleVisibleNormal(V_local, alpha_x, alpha_y);
    }

    float cos_theta_i = dot(V_local, N_micro_local);

    // 计算反射和折射的比例
    float R;
    if (material.type == 0) {
        // 电介质
        R = Fresnel(cos_theta_i, material.IOR);
    } else {
        // 导体
        R = ComplexFresnel(cos_theta_i, material.IOR, material.K);
    }
    float T = 1 - R;

    // 计算采样折射和反射的概率
    float P_R = R;
    float P_T = T;
    if (material.type == 1) {
        // 折射光在导体表面会被快速吸收，所以导体不采样折射
        P_T = 0;
    }
    P_R = P_R / (P_R + P_T);
    P_T = 1 - P_R;

    if (rnd(ray.rnd_state) < P_R) {
        // 采样反射
        if (is_smooth) {
            L_local = vec3(-V_local.x, V_local.y, -V_local.z);
        } else {
            L_local = normalize(-V_local + 2 * dot(V_local, N_micro_local) * N_micro_local);
            // 处理入射光和反射光不在同一半球的情况
            if (V_local.y * L_local.y <= 0) {
                ray.end = true;
                return;
            }

            bsdf /= 4 * L_local.y * V_local.y;
            pdf /= 4 * abs(dot(V_local, N_micro_local));
        }

        bsdf *= R;
        pdf *= P_R;
    } else {
        // 采样折射
        vec4 result = Refract(V_local, N_micro_local, cos_theta_i, material.IOR);
        if (result.w == 0) {
            // 全反射
            ray.end = true;
            return;
        }
        L_local = result.xyz;
        float IOR = result.w;

        if (!is_smooth) {
            float sqrt_denom = dot(L_local, N_micro_local) + dot(V_local, N_micro_local) / IOR;
            float denom = sqrt_denom * sqrt_denom;

            bsdf *= abs(dot(V_local, N_micro_local) * dot(V_local, N_micro_local) / (L_local.y * V_local.y * denom));
            pdf *= abs(dot(V_local, N_micro_local)) / denom;
        }

        bsdf *= T / (IOR * IOR);
        pdf *= P_T;
    }

    if (!is_smooth) {
        bsdf *= D(N_micro_local, alpha_x, alpha_y) * G2(V_local, L_local, alpha_x, alpha_y) * abs(L_local.y);
        pdf *= DPDF(V_local, N_micro_local, alpha_x, alpha_y);
    }
    if (isnan(bsdf) || isnan(pdf) || pdf < 1e-6) {
        ray.end = true;
        return;
    }

    ray.albedo *= material.albedo * bsdf / pdf;
    ray.origin = pos;
    ray.direction = normalize(L_local.x * X + L_local.y * normal + L_local.z * Z);
}
