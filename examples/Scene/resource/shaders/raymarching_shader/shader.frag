#version 450

layout (location = 0) out vec4 out_color;

layout (binding = 0) uniform Camera {
    vec3 pos;
    mat4 view;
    mat4 project;
} camera;

layout (binding = 1) uniform RenderData {
    vec2 window_size;
    int max_steps;
    float max_dist;
    float epsillon_dist;
    vec4 sphere;
    vec3 light;
} data;

float compute_dist(vec3 point) {
    return min(length(point - data.sphere.xyz) - data.sphere.w, point.y);
}

vec3 compute_normal(vec3 point) {
    float dist = compute_dist(point);
    vec3 n = dist - vec3(
        compute_dist(point - vec3(data.epsillon_dist, 0, 0)),
        compute_dist(point - vec3(0, data.epsillon_dist, 0)),
        compute_dist(point - vec3(0, 0, data.epsillon_dist))
    );
    return normalize(n);
}

float ray_match(vec3 pos, vec3 direction) {
    float dist = 0.;
    for(int i = 0; i < data.max_steps; i ++) {
        vec3 point = pos + dist * direction;
        float min_dist = compute_dist(point);
        dist += min_dist;
        if(dist > data.max_dist || min_dist < data.epsillon_dist)
            break;
    }
    return dist;
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * data.window_size) / data.window_size.y;
    uv.y = -uv.y;
    vec3 view_direction = normalize(inverse(camera.view) * vec4(uv, -1, 0)).xyz;
    float dist = ray_match(camera.pos, view_direction);
    vec3 pos = camera.pos + view_direction * dist;

    vec3 L_D = data.light - pos;
    float L_L = length(L_D);
    vec3 L = normalize(L_D);
    vec3 N = compute_normal(pos);

    out_color = vec4(vec3(max(dot(L, N), 0)) / (L_L * L_L), 1);
    if (dist < data.max_dist) {
        out_color.xyz += vec3(0.1);
    }
    if (ray_match(pos + N * data.epsillon_dist * 2, L) < L_L) {
        out_color.xyz *= 0.1;
    }
}
