layout (binding = 6) uniform VolumeRenderingArgs {
    float sigma_maj_uniform;
    float sigma_a_uniform;
    float sigma_s_uniform;
    float g_uniform;

    float sigma_maj_perlin;
    float sigma_a_perlin;
    float sigma_s_perlin;
    float g_perlin;
    float L;
    float H;
    float freq;
    int OCT;

    float sigma_maj_cloud;
    float sigma_a_cloud;
    float sigma_s_cloud;
    float g_cloud;

    float sigma_maj_smoke;
    float sigma_a_smoke;
    float sigma_s_smoke;
    float g_smoke;
} args;
