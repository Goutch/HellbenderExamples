#version 460
#extension GL_EXT_ray_tracing: enable
#extension GL_EXT_nonuniform_qualifier: enable
#extension GL_GOOGLE_include_directive : require
#include "../../print.glsl"
#define PAYLOAD_IN
#include "../common.glsl"
hitAttributeEXT HitResult
{
    vec2 barycentric_coords;
} hitResult;
#include "../raytracing.glsl"


#define IS_CENTER_PIXEL_CONDITION gl_LaunchIDEXT.x == (gl_LaunchSizeEXT.x) / 2 && gl_LaunchIDEXT.y == (gl_LaunchSizeEXT.y / 2)
#define PRINT debugPrintfEXT
void main()
{
    InstanceInfo instance_info = instances.infos[gl_InstanceCustomIndexEXT];

    //Get the barycentric coordinates of the hit point.
    vec3 baryCoords = vec3(hitResult.barycentric_coords.x, hitResult.barycentric_coords.y, 1.0 - (hitResult.barycentric_coords.x + hitResult.barycentric_coords.y));

    // Fetch the triangle indices.// Primitive ID of the intersected triangle.

    uvec3 triangle_indices;
    if (instance_info.indices_size==16)
    {
        uint first_index = gl_PrimitiveID*3;
        if (mod(gl_PrimitiveID, 2)==0)
        {
            first_index = first_index/2;
            uint index1and2 = mesh_indices_buffers[gl_InstanceID].indices[nonuniformEXT(first_index)];
            uint index3and4 = mesh_indices_buffers[gl_InstanceID].indices[nonuniformEXT(first_index + 1)];
            triangle_indices = uvec3(
            index1and2 & uint(0x0000FFFF),
            (index1and2 & uint(0xFFFF0000)) >> 16,
            index3and4 & uint(0x0000FFFF));
        }
        else
        {
            first_index -= 1;
            first_index = first_index/2;
            uint index0and1 = mesh_indices_buffers[gl_InstanceID].indices[nonuniformEXT(first_index)];
            uint index2and3 = mesh_indices_buffers[gl_InstanceID].indices[nonuniformEXT(first_index + 1)];

            triangle_indices = uvec3(
            (index0and1 & uint(0xFFFF0000)) >> 16,
            index2and3 & uint(0x0000FFFF),
            (index2and3 & uint(0xFFFF0000))>> 16);
        }

    }
    else if (instance_info.indices_size==32)
    {
        triangle_indices = uvec3(
        mesh_indices_buffers[gl_InstanceID].indices[nonuniformEXT((gl_PrimitiveID*3))],
        mesh_indices_buffers[gl_InstanceID].indices[nonuniformEXT((gl_PrimitiveID*3)+1)],
        mesh_indices_buffers[gl_InstanceID].indices[nonuniformEXT((gl_PrimitiveID*3)+2)]);
    }

    //// Fetch the vertex normals.
    vec3 n0 = vec3(mesh_normals_buffers[gl_InstanceID].normals[nonuniformEXT(triangle_indices.x*3)],
    mesh_normals_buffers[gl_InstanceID].normals[nonuniformEXT((triangle_indices.x*3) +1)],
    mesh_normals_buffers[gl_InstanceID].normals[nonuniformEXT((triangle_indices.x*3) +2)]);
    vec3 n1 = vec3(mesh_normals_buffers[gl_InstanceID].normals[nonuniformEXT(triangle_indices.y*3)],
    mesh_normals_buffers[gl_InstanceID].normals[nonuniformEXT((triangle_indices.y*3) +1)],
    mesh_normals_buffers[gl_InstanceID].normals[nonuniformEXT((triangle_indices.y*3) +2)]);
    vec3 n2 = vec3(mesh_normals_buffers[gl_InstanceID].normals[nonuniformEXT(triangle_indices.z*3)],
    mesh_normals_buffers[gl_InstanceID].normals[nonuniformEXT((triangle_indices.z*3) +1)],
    mesh_normals_buffers[gl_InstanceID].normals[nonuniformEXT((triangle_indices.z*3) +2)]);

    //// Interpolate the normals using the barycentric coordinates.
    vec3 interpolatedNormal = normalize(
    (baryCoords.x * n1) +
    (baryCoords.y * n2) +
    (baryCoords.z * n0));


    vec2 uv0 = mesh_tex_coords_buffers[gl_InstanceID].coords[nonuniformEXT(triangle_indices.x)];
    vec2 uv1 = mesh_tex_coords_buffers[gl_InstanceID].coords[nonuniformEXT(triangle_indices.y)];
    vec2 uv2 = mesh_tex_coords_buffers[gl_InstanceID].coords[nonuniformEXT(triangle_indices.z)];
    vec2 interpolatedUVs =  (baryCoords.x * uv1) +
    (baryCoords.y * uv2) +
    (baryCoords.z * uv0);
    //if (IS_CENTER_PIXEL_CONDITION)
    //{
    //    PRINT("Normal: (%f, %f, %f)\n", baryCoords.x, baryCoords.y, ((1.0 - (baryCoords.x +baryCoords.y))));
    //}


    traceRays(interpolatedNormal, interpolatedUVs);
}

