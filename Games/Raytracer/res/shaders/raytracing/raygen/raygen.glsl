#version 460
#extension GL_EXT_ray_tracing: enable
#extension GL_GOOGLE_include_directive: require
#extension GL_EXT_nonuniform_qualifier: require
#define HISTORY_COUNT 8
#define PAYLOAD_OUT
#include "../common.glsl"


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
vec2 calculateVelocity(vec3 ray_dir, vec3 ray_origin)
{
    if (frame.index -1 < 0)
    return vec2(0);
    int previous_history_index = int(mod(frame.index - 1, HISTORY_COUNT));

    mat4 last_view = inverse(camera_history.properties[previous_history_index].transform);
    mat4 last_proj = camera_history.properties[previous_history_index].projection;

    mat4 last_view_proj =  last_proj * last_view;
    vec4 prev_screen_space = last_view_proj * vec4((primaryRayPayload.payload.hit_t *ray_dir) + ray_origin, 1.0);
    vec2 prev_uv = 0.5 * (prev_screen_space.xy / prev_screen_space.w) + 0.5;
    vec2 uv = (vec2(gl_LaunchIDEXT.xy) + vec2(0.5)) / vec2(gl_LaunchSizeEXT.xy);
    vec2 velocity = (uv - prev_uv);

    return velocity.xy;
}
void main()
{
    int history_index = int(mod(frame.index, HISTORY_COUNT));
    const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 inUV = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
    vec2 d = inUV * 2.0 - 1.0;

    vec2 offset= offsets[int(mod(frame.index, 16))];
    offset.x = ((offset.x - 0.5f) / gl_LaunchSizeEXT.x) * 2;
    offset.y = ((offset.y - 0.5f) / gl_LaunchSizeEXT.y) * 2;
    mat4 camera_transform = camera_history.properties[history_index].transform;
    mat4 camera_view_mat = inverse(camera_transform);
    mat4 camera_proj_mat = camera_history.properties[history_index].projection;

    vec4 origin = camera_transform * vec4(0, 0, 0, 1);
    vec4 target_jittered = inverse(camera_proj_mat) * vec4(d + offset, 1, 1);
    vec4 target = inverse(camera_proj_mat) * vec4(d.x, d.y, 1, 1);

    vec4 direction = normalize(camera_transform * vec4(normalize(target.xyz), 0));
    vec4 direction_jittered = normalize(camera_transform * vec4(normalize(target_jittered.xyz), 0));
    float tmin = 0.001;
    float tmax = 1000.0;

    vec3 color= vec3(0.0, 0.0, 0.0);

    float x = frame.index * PHI;
    x= mod(x, 1.0);
    x*=1000;

    primaryRayPayload.payload.rng_state = uint(uint(gl_LaunchIDEXT.x) * uint(1973) + uint(gl_LaunchIDEXT.y) * uint(9277) + uint(floor(x)) * uint(26699)) | uint(1);
    primaryRayPayload.payload.bounce_count = 0;
    primaryRayPayload.payload.color = vec3(0.0, 0.0, 0.0);
    primaryRayPayload.payload.hit_sky = false;
    primaryRayPayload.payload.hit_t = 0.0;
    primaryRayPayload.payload.noise_sample_count = 0;
    traceRayEXT(topLevelAS, //topLevelacceleationStructure
    gl_RayFlagsOpaqueEXT, //rayFlags
    0xff, //cullMask
    0, //sbtRecordOffset
    1, //sbtRecordStride
    0, //missIndex
    origin.xyz, //origin
    tmin, //Tmin
    direction_jittered.xyz, //direction
    tmax, //Tmax
    0);//payload location
    color += primaryRayPayload.payload.color;

    vec2 velocity = calculateVelocity(direction.xyz, origin.xyz);

    imageStore(historyAlbedo[history_index], ivec2(gl_LaunchIDEXT.xy), vec4(color, 1.0));
    imageStore(historyNormalDepth[history_index], ivec2(gl_LaunchIDEXT.xy), vec4(abs(primaryRayPayload.payload.hit_normal), 1 - (primaryRayPayload.payload.hit_t / tmax)));
    imageStore(historyMotion[history_index], ivec2(gl_LaunchIDEXT.xy), vec4(velocity, 0.0, 1.0));

    ivec2 texture_space_coord = ivec2(gl_LaunchIDEXT.xy);

    vec2 uv = ((vec2(gl_LaunchIDEXT.xy) +vec2(0.5)) / vec2(gl_LaunchSizeEXT.xy));
    uint historySampleCount = 1;

    for (int i = 1; i < HISTORY_COUNT-1; i++)
    {
        if (frame.index - i < 0)
        break;
        int current_history_index = int(mod(frame.index -i, HISTORY_COUNT));

        texture_space_coord = ivec2(floor(uv * vec2(gl_LaunchSizeEXT.xy)));
        uv -= imageLoad(historyMotion[current_history_index], texture_space_coord).xy;
        if (max(uv.x, uv.y) > 1.0 || min(uv.x, uv.y) < 0.0)
        {
            color += vec3(1, 0, 0);
            historySampleCount++;
            continue;
        }

        texture_space_coord = ivec2(floor(uv* vec2(gl_LaunchSizeEXT.xy)));
        color += imageLoad(historyAlbedo[current_history_index], texture_space_coord).rgb;

        historySampleCount++;
    }

    color /= float(historySampleCount);
    const float gamma = 2.0;
    float exposure = 1.0;
    //tone mapping
    vec3 mapped = vec3(1.0) - exp(- color * exposure);
    //exposure
    mapped = pow(mapped, vec3(1.0 / gamma));

    imageStore(outputAlbedo, ivec2(gl_LaunchIDEXT.xy), vec4(mapped, 1.0));
}
