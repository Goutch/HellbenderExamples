#version 460
#extension GL_EXT_ray_tracing: enable
#extension GL_EXT_nonuniform_qualifier: enable
#extension GL_GOOGLE_include_directive : require
#define PRIMARY_PAYLOAD_IN
#include "../common.glsl"
#include "mesh.glsl"

void main()
{
    InstanceInfo instance_info = instances.infos[gl_InstanceID];
    VertexData vertexData = getInterpolatedVertexData(instance_info);

    vec2 uvs = vertexData.tex_coords;
    vec3 normal = vertexData.normal;

    payload.hit_uv = uvs;
    payload.hit_material = instance_info.material_index;
    payload.hit_normal = normal;
    payload.hit_t = gl_HitTEXT;
    payload.hit_sky = false;
}

