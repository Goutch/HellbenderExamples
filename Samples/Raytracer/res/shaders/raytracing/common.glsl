#include "../data_structures.glsl"
#define HISTORY_COUNT 8
const float T_MIN = 0.01;
const float E = 2.71828182845904523536028747135266249;
const float PHI = 1.61803398874989484820459;// Φ = Golden Ratio
const float PI = 3.1415926535897932384626433832795;
const float TWO_PI = 2.0 * PI;

layout (binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
#ifdef HISTORY_COUNT
layout (binding = 1, set = 0, rgba32f) uniform image2D historyAlbedo[HISTORY_COUNT];
layout (binding = 2, set = 0, rgba32f) uniform image2D historyNormalDepth[HISTORY_COUNT];
layout (binding = 3, set = 0, rgba32f) uniform image2D historyMotion[HISTORY_COUNT];
layout (binding = 4, set = 0, rgba32f) uniform image2D historyPositionRoughness[HISTORY_COUNT];
layout (binding = 5, set = 0, rgba32f) uniform image2D historyIrradiance[HISTORY_COUNT];
layout (binding = 6, set = 0) uniform CameraHistory
{
    CameraProperties properties[HISTORY_COUNT];
} camera_history;
#endif

layout (binding =7, set = 0) uniform FrameUBO {
    Frame data;
} frame;

layout (binding = 8, set = 1, std430) readonly buffer MaterialDataBuffer
{
    MaterialData materials[];
} materials;
layout (binding = 9, set = 2, std430) readonly buffer InstanceInfos
{
    InstanceInfo infos[];
} instances;
layout (binding = 10, set = 3) uniform sampler2D textures[];

layout (binding = 11, set = 4, std430) readonly buffer MeshIndicesBuffers
{
    uint indices[];
} mesh_indices_buffers[];
layout (binding = 12, set = 5, std430) readonly buffer MeshTexCoordsBuffers
{
    vec2 coords[];
} mesh_tex_coords_buffers[];

layout (binding = 13, set = 6, std430) readonly buffer MeshNormalsBuffers
{
    float normals[];
} mesh_normals_buffers[];

#ifdef PRIMARY_PAYLOAD_IN
layout (location = 0) rayPayloadInEXT PrimaryRayPayLoad
{
    PrimaryRayData payload;
};
#endif
#ifdef  PRIMARY_PAYLOAD_OUT
layout(location = 0) rayPayloadEXT PrimaryRayPayLoad
{
    PrimaryRayData payload;
};
#endif

