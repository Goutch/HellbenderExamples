#version 460
#extension GL_EXT_ray_tracing: enable
#extension GL_EXT_nonuniform_qualifier: enable
#extension GL_GOOGLE_include_directive : require
#define PRIMARY_PAYLOAD_IN
#include "../common.glsl"
#include "raytracing.glsl"
#include "mesh.glsl"

void main()
{
    InstanceInfo instance_info = instances.infos[gl_InstanceID];
    VertexData vertexData = getInterpolatedVertexData(instance_info);
    MaterialData material  = materials.materials[instance_info.material_index];
    vec2 uvs = vertexData.tex_coords;
    vec3 normal = vertexData.normal;

    if (material.normal_index>=0)
    {
        vec3 normalMap = texture(textures[nonuniformEXT(material.normal_index)], uvs).rgb;
        normal = normalize(normal + normalMap);
    }
    vec4 albedo = material.albedo;
    if (material.albedo_index>=0)
    {
        albedo *= texture(textures[nonuniformEXT(material.albedo_index)], uvs);
    }
    vec3 hit_position = gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;
    int bounce_count = payload.bounce_count;
    if (bounce_count<frame.data.max_bounces)
    {
        payload.bounce_count++;
        payload.irradiance = traceSecondaryRay(gl_WorldRayDirectionEXT, hit_position, material, normal, albedo);
        payload.bounce_count = bounce_count;
    }

    payload.hit_normal = normal;
    payload.albedo = albedo.rgb;
    payload.hit_t = gl_HitTEXT;
    payload.hit_sky = false;
}

