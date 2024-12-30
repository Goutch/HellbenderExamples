struct MaterialData
{
    vec4 albedo;
    vec4 emission;
    float roughness;
};

struct PayloadData{
    vec3 color;
    int bounce_count;
    uint noise_sample_count;
    uint rng_state;
    bool hit_sky;
    vec3 hit_normal;
    float hit_t;
    vec3 hit_velocity;
};

const float E = 2.71828182845904523536028747135266249;
const float PHI = 1.61803398874989484820459;// Φ = Golden Ratio
const float PI = 3.1415926535897932384626433832795;
const float TWO_PI = 2.0 * PI;

layout (binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout (binding = 1, set = 0, rgba32f) uniform writeonly image2D outputAlbedo;
layout (binding = 2, set = 0, rgba32f) uniform writeonly image2D outputNormalDepth;
layout (binding = 3, set = 0, rgba32f) uniform image2D outputMotion;
#ifdef HISTORY_COUNT
layout (binding = 4, set = 0, rgba32f) uniform image2D historyAlbedo[HISTORY_COUNT];
layout (binding = 5, set = 0, rgba32f) uniform image2D historyNormalDepth[HISTORY_COUNT];
layout (binding = 6, set = 0, rgba32f) uniform image2D historyMotion[HISTORY_COUNT];
#endif

layout (binding = 7, set = 0) uniform CameraProperties
{
    mat4 view_inverse;
    mat4 projection_inverse;
} cam;


layout (binding = 8, set = 0) uniform Frame {
    float time;
    uint index;
    uint sample_count;
    uint max_bounces;
    float scattering_multiplier;
    float density_falloff;
    int use_blue_noise;
} frame;

layout (binding = 9, set = 0, std430) readonly buffer MaterialDataBuffer
{
    MaterialData materials[];
} materials;

layout (binding = 10, set = 0, rgba8ui) uniform readonly uimage2D blueNoise;
layout (binding = 11, set = 0) uniform LastCameraProperties
{
    mat4 view;
    mat4 projection;
} last_cam;

struct vertexUVNormal
{
    vec3 position;
    vec2 uv;
    vec3 normal;
};
layout (binding = 12, set = 1, std430) readonly buffer MeshesTexCoords
{
    vec2 coords[];
} meshes_tex_coords;

layout (binding = 13, set = 2, std430) readonly buffer MeshesNormals
{
    vec2 normals[];
} meshes_normals;

#ifdef PAYLOAD_IN
layout (location = 0) rayPayloadInEXT PrimaryRayPayLoad
{
    PayloadData payload;
} primaryRayPayload;
#endif
#ifdef PAYLOAD_OUT
layout(location = 0) rayPayloadEXT PrimaryRayPayLoad
{
    PayloadData payload;
} primaryRayPayload;
#endif


