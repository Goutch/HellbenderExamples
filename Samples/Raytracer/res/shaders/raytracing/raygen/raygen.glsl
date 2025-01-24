#version 460
#extension GL_EXT_ray_tracing: enable
#extension GL_GOOGLE_include_directive: require
#extension GL_EXT_nonuniform_qualifier: require
#extension GL_KHR_shader_subgroup_basic : require

#define PRIMARY_PAYLOAD_OUT
#include "../common.glsl"
#include "raytracing.glsl"
vec2 offsets[16] = {
vec2(0.500000, 0.333333),
vec2(0.250000, 0.666667),
vec2(0.750000, 0.111111),
vec2(0.125000, 0.444444),
vec2(0.625000, 0.777778),
vec2(0.375000, 0.222222),
vec2(0.875000, 0.555556),
vec2(0.062500, 0.888889),
vec2(0.562500, 0.037037),
vec2(0.312500, 0.370370),
vec2(0.812500, 0.703704),
vec2(0.187500, 0.148148),
vec2(0.687500, 0.481481),
vec2(0.437500, 0.814815),
vec2(0.937500, 0.259259),
vec2(0.031250, 0.592593),
};
struct Ray{
    vec3 origin;
    vec3 direction;
};
Ray getRay(bool jitter, int history_index)
{

    const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 inUV = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
    vec2 centered_uvs = inUV * 2.0 - 1.0;

    mat4 camera_transform = camera_history.properties[history_index].transform;
    mat4 camera_view_mat = inverse(camera_transform);
    mat4 camera_proj_mat = camera_history.properties[history_index].projection;

    vec4 origin = camera_transform * vec4(0, 0, 0, 1);
    vec4 target;
    if (jitter)
    {
        vec2 offset= offsets[int(mod(frame.data.index, 16))];
        offset.x = ((offset.x - 0.5f) / gl_LaunchSizeEXT.x) * 2;
        offset.y = ((offset.y - 0.5f) / gl_LaunchSizeEXT.y) * 2;
        target = inverse(camera_proj_mat) * vec4(centered_uvs + (offset * int(jitter)), 1, 1);
    }
    else
    {
        target = inverse(camera_proj_mat) * vec4(centered_uvs, 1, 1);
    }
    vec4 direction = normalize(camera_transform * vec4(normalize(target.xyz), 0));

    return Ray(origin.xyz, direction.xyz);
}
uint getRNGState()
{
    float x = float(frame.data.index)* PHI;
    x= mod(x, 1.0);
    x*=1000;

    return uint(uint(gl_LaunchIDEXT.x) * uint(1973) + uint(gl_LaunchIDEXT.y) * uint(9277) + uint(floor(x)) * uint(26699)) | uint(1);
}

vec2 calculateVelocity(vec3 ray_dir, vec3 ray_origin)
{
    if (frame.data.index -1 < 0)
    return vec2(0);
    int previous_history_index = int(mod(frame.data.index - 1, HISTORY_COUNT));

    mat4 last_view = inverse(camera_history.properties[previous_history_index].transform);
    mat4 last_proj = camera_history.properties[previous_history_index].projection;

    mat4 last_view_proj =  last_proj * last_view;
    vec4 prev_screen_space = last_view_proj * vec4((payload.hit_t *ray_dir) + ray_origin, 1.0);
    vec2 prev_uv = 0.5 * (prev_screen_space.xy / prev_screen_space.w) + 0.5;
    vec2 uv = (vec2(gl_LaunchIDEXT.xy) + vec2(0.5)) / vec2(gl_LaunchSizeEXT.xy);
    vec2 velocity = (uv - prev_uv);

    return velocity.xy;
}

vec3 getMaterialAlbedo(MaterialData material, vec2 uvs)
{
    vec4 albedo = material.albedo;
    if (material.albedo_index >= 0)
    {
        albedo *= texture(textures[nonuniformEXT(material.albedo_index)], uvs);
    }
    return albedo.rgb;
}
vec3 getMaterialNormal(vec3 normal, MaterialData material, vec2 uvs)
{
    if (material.normal_index >= 0)
    {
        vec3 normalMap = texture(textures[nonuniformEXT(material.normal_index)], uvs).rgb;
        normal = normalize(normal + normalMap);
    }
    return normal;
}

void main()
{
    int history_index = int(mod(frame.data.index, HISTORY_COUNT));
    uint global_thread_id = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x;
    uint thread_group_id = global_thread_id / gl_SubgroupSize;

    Ray primary_ray = getRay(true, history_index);

    float tmin = 0.001;
    float tmax = 1000.0;

    vec3 primary_albedo = vec3(0, 0, 0);
    vec4 velocity = vec4(0, 0, 0, 1.0);
    vec4 normalDepth = vec4(0, 0, 0, 1.0);
    vec4 irradiance = vec4(0, 0, 0, 1.0);

    float sample_weight = 1.0 / float(frame.data.sample_count);
    payload.rng_state = getRNGState();

    for (int sample_index = 0; sample_index < frame.data.sample_count; sample_index++)
    {
        payload.irradiance = vec3(0.0, 0.0, 0.0);
        payload.hit_sky = false;
        payload.hit_t = 0.0;
        payload.hit_normal = vec3(0.0, 1.0, 0.0);

        traceRayEXT(topLevelAS, //topLevelacceleationStructure
        gl_RayFlagsOpaqueEXT, //rayFlags
        0xff, //cullMask
        0, //sbtRecordOffset
        1, //sbtRecordStride
        0, //missIndex
        primary_ray.origin.xyz, //origin
        tmin, //Tmin
        primary_ray.direction, //direction
        tmax, //Tmax
        0);//payload location

        if (payload.hit_sky)
        {
            normalDepth = vec4(payload.hit_normal, 1 - (payload.hit_t / tmax));
            irradiance += vec4(payload.irradiance, 1.0);
            primary_albedo += irradiance.rgb;
            break;
        }

        //did not hit the sky so there is a material
        MaterialData material  = materials.materials[payload.hit_material];
        //extract normal from normal texture
        vec3 normal = getMaterialNormal(payload.hit_normal, material, payload.hit_uv);
        //extract albedo from albedo texture
        vec3 albedo = getMaterialAlbedo(material, payload.hit_uv);
        normalDepth = vec4(normal, 1 - (payload.hit_t / tmax));

        primary_albedo += albedo * sample_weight;

        vec3 incoming_ray_dir  = primary_ray.direction;
        vec3 incoming_ray_origin = primary_ray.origin;
        for (int bounce = 0; bounce < frame.data.max_bounces;bounce++)
        {
            vec3 hit_position = incoming_ray_origin + payload.hit_t * incoming_ray_dir;

            vec3 irradianceAlbedo = albedo;
            if (bounce==0)
            {
                irradianceAlbedo = vec3(1.0);
            }
            payload.irradiance = traceSecondaryRay(incoming_ray_dir,
            hit_position.xyz,
            material,
            normal.xyz,
            irradianceAlbedo, bounce == 0);
            if (payload.hit_sky)
            {
                break;
            }
            material  = materials.materials[payload.hit_material];
            //extract normal from normal texture
            normal = getMaterialNormal(payload.hit_normal, material, payload.hit_uv);
            //extract albedo from albedo texture
            albedo = getMaterialAlbedo(material, payload.hit_uv);
            incoming_ray_origin = hit_position;
        }

        irradiance += vec4(payload.irradiance, 1.0) *sample_weight;
    }

    velocity = vec4(calculateVelocity(primary_ray.direction, primary_ray.origin.xyz), 0.0, 1.0);
    vec4 primary_hit_position = vec4(primary_ray.origin.xyz + (normalDepth.w* primary_ray.direction), 1.0);

    imageStore(historyAlbedo[history_index], ivec2(gl_LaunchIDEXT.xy), vec4(primary_albedo, 1.0));
    imageStore(historyNormalDepth[history_index], ivec2(gl_LaunchIDEXT.xy), normalDepth);
    imageStore(historyMotion[history_index], ivec2(gl_LaunchIDEXT.xy), velocity);
    imageStore(historyIrradiance[history_index], ivec2(gl_LaunchIDEXT.xy), irradiance);
    imageStore(historyPosition[history_index], ivec2(gl_LaunchIDEXT.xy), primary_hit_position);
}
