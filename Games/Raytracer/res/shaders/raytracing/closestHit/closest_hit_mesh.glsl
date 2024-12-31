#version 460
#extension GL_EXT_ray_tracing: enable
#extension GL_EXT_nonuniform_qualifier: enable
#extension GL_GOOGLE_include_directive : require

#define PAYLOAD_IN
#include "../common.glsl"
hitAttributeEXT HitResult
{
    vec3 barycentric_coords;
} hitResult;
#include "../raytracing.glsl"

void main()
{
    InstanceInfo instance_info = instances.infos[gl_InstanceCustomIndexEXT];

    // Get the barycentric coordinates of the hit point.
    // vec3 baryCoords = hitResult.barycentric_coords;

    // Fetch the triangle indices.// Primitive ID of the intersected triangle.
    // uvec3 triangle = mesh_indices[gl_InstanceID][nonuniformEXT(gl_PrimitiveID)];

    // Fetch the vertex normals.
    // vec3 n0 = mesh_normals[gl_InstanceID][nonuniformEXT(triangle.x)];
    // vec3 n1 = mesh_normals[gl_InstanceID][nonuniformEXT(triangle.y)];
    // vec3 n2 = mesh_normals[gl_InstanceID][nonuniformEXT(triangle.z)];

    // Interpolate the normals using the barycentric coordinates.
    // vec3 interpolatedNormal = normalize(
    //  baryCoords.x * n0 +
    //  baryCoords.y * n1 +
    //baryCoords.z * n2
    //);
    traceRays(vec3(0.0, 1.0, 0.0));
}

