#version 460
#extension GL_EXT_ray_tracing: enable
#extension GL_EXT_nonuniform_qualifier: enable
#extension GL_GOOGLE_include_directive : require

#define PRIMARY_PAYLOAD_IN
#include "../common.glsl"
#include "raytracing.glsl"
hitAttributeEXT HitResult
{
    vec3 normal;
} hitResult;


void main()
{
    InstanceInfo instance_info = instances.infos[gl_InstanceID];
    MaterialData material  = materials.materials[instance_info.material_index];

    vec4 albedo = material.albedo;
    if (material.albedo_index>=0)
    {
        albedo *= texture(textures[nonuniformEXT(material.albedo_index)], vec2(0.0, 0.0));
    }
    vec3 normal = hitResult.normal;

    vec3 incoming_ray_dir = gl_WorldRayDirectionEXT;
    vec3 hit_position = gl_WorldRayOriginEXT + gl_HitTEXT * incoming_ray_dir;

    int bounce_count =payload.bounce_count;
    if (bounce_count<frame.data.max_bounces)
    {
        vec4 irradianceAlbedo = albedo;
        if (bounce_count==0)
        {
            irradianceAlbedo = vec4(1.0);
        }

        payload.bounce_count++;
        payload.irradiance = traceSecondaryRay(incoming_ray_dir.xyz, hit_position.xyz, material, normal.xyz, irradianceAlbedo);
    }
    payload.bounce_count = bounce_count;
    payload.hit_normal = normal;
    payload.albedo = albedo.rgb;
    payload.hit_t = gl_HitTEXT;
    payload.hit_sky = false;
}

