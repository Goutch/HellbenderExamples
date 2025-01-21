struct Frame {
    float time;
    uint index;
    uint sample_count;
    uint max_bounces;
    float scattering_multiplier;
    float density_falloff;
    float exposure;
    float gamma;
};
struct MaterialData
{
    vec4 albedo;
    vec4 emission;
    int albedo_index;
    int normal_index;
    float roughness;
};
struct InstanceInfo
{
    uint material_index;
    uint mesh_index;
    uint indices_size;
};

struct CameraProperties
{
    mat4 transform;
    mat4 projection;
};

struct PrimaryRayData{
    vec3 irradiance;
    vec3 albedo;
    uint rng_state;
    bool hit_sky;
    vec3 hit_normal;
    float hit_t;
    int bounce_count;
};