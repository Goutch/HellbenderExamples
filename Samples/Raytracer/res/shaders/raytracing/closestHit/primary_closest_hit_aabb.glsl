#version 460
#extension GL_EXT_ray_tracing: enable
#extension GL_EXT_nonuniform_qualifier: enable
#extension GL_GOOGLE_include_directive : require

#define PRIMARY_PAYLOAD_IN
#include "../common.glsl"
hitAttributeEXT HitResult
{
    vec3 normal;
} hitResult;


void main()
{
    InstanceInfo instance_info = instances.infos[gl_InstanceID];

    vec3 normal = hitResult.normal;

    payload.hit_material = instance_info.material_index;
    payload.hit_uv = vec2(0.0, 0.0);
    payload.hit_normal = normal;
    payload.hit_t = gl_HitTEXT;
    payload.hit_sky = false;
}

