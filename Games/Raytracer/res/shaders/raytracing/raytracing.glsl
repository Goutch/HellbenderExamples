//https://github.com/KhronosGroup/GLSL/blob/main/extensions/nv/GLSL_NV_ray_tracing.txt
const float LIGHT_BIAS = 0.3;
vec3 sampleBlueNoise(ivec2 coord)
{
    vec4 noise = imageLoad(blueNoise, coord);
    return vec3(noise.rgb)/vec3(255.0);
}
uint wang_hash(inout uint seed)
{
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

float RandomFloat01(inout uint state)
{
    return float(wang_hash(state)) / 4294967296.0;
}

vec3 RandomUnitVectorRNG(inout uint state)
{
    float z = RandomFloat01(state) * 2.0f - 1.0f;
    float a = RandomFloat01(state) * TWO_PI;
    float r = sqrt(1.0f - z * z);
    float x = r * cos(a);
    float y = r * sin(a);
    return vec3(x, y, z);
}
vec3 RandomUnitVectorNoise()
{
    ivec2 coord = ivec2(gl_LaunchIDEXT.xy);
    float index = float(frame.index+primaryRayPayload.payload.noise_sample_count);
    vec2 offset = vec2(index*PHI, index*E);

    offset=mod(offset, 1.0);
    offset *= 469.0;
    coord += ivec2(offset);
    coord = ivec2(mod(vec2(coord), vec2(469)));
    primaryRayPayload.payload.noise_sample_count++;
    vec3 noise = sampleBlueNoise(coord);
    return normalize(noise-vec3(0.5));
}
vec3 RandomDiffuseVectorFromNoise(vec3 normal)
{
    return normalize(RandomUnitVectorNoise() + normal);
}
vec3 RandomDiffuseVectorFromRNG(vec3 normal)
{
    return normalize(RandomUnitVectorRNG(primaryRayPayload.payload.rng_state)+normal);
}
vec3 RandomSpecularVector(vec3 direction, vec3 diffuse, float roughness)
{
    return normalize(mix(direction, diffuse, roughness*roughness));
}

void traceLight(vec3 origin, vec3 dir, float tmax)
{
    const float tmin = 0.0001;
    traceRayEXT(topLevelAS, //topLevelacceleationStructure
    gl_RayFlagsOpaqueEXT|gl_RayFlagsSkipClosestHitShaderEXT|  gl_RayFlagsTerminateOnFirstHitEXT, //rayFlags
    0xFF, //cullMask
    0, //sbtRecordOffset
    1, //sbtRecordStride
    0, //missIndex
    origin, //origin
    tmin, //Tmin
    dir, //direction
    tmax, //Tmax
    0);//payload location
}
void trace(vec3 origin, vec3 dir, float tmax)
{
    const float tmin = 0.0001;
    primaryRayPayload.payload.color=vec3(0.0);
    traceRayEXT(topLevelAS, //topLevelacceleationStructure
    gl_RayFlagsOpaqueEXT, //rayFlags
    0xFF, //cullMask
    0, //sbtRecordOffset
    1, //sbtRecordStride
    0, //missIndex
    origin, //origin
    tmin, //Tmin
    dir, //direction
    tmax, //Tmax
    0);//payload location
}
//------------------------------------ UNIFORMS ------------------------------------
void traceRays(vec3 normal, vec2 uvs)
{
    InstanceInfo instance_info = instances.infos[gl_InstanceID];
    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

    primaryRayPayload.payload.bounce_count++;
    uint numSamples=1;
    if (frame.sample_count<1)
    {
        numSamples = uint(mod(gl_LaunchIDEXT.x+gl_LaunchIDEXT.y+mod(frame.index, 2), 2));
    }
    else
    {
        numSamples = primaryRayPayload.payload.bounce_count==1?frame.sample_count:1;
    }
    MaterialData material  = materials.materials[instance_info.material_index];

    vec3 to_light_dir= normalize(vec3(sin(frame.time), sin(frame.time), cos(frame.time)));
    float light_dot_product=dot(normal, to_light_dir);
    vec3 color =vec3(0.0);
    int num_sun_samples = 0;
    if (primaryRayPayload.payload.bounce_count<frame.max_bounces)
    {
        for (uint i = 0; i < numSamples; i++)
        {
            vec3 newDir;
            if (material.roughness>0.999 && smoothstep(0, 1, light_dot_product)>RandomFloat01(primaryRayPayload.payload.rng_state))
            {

                primaryRayPayload.payload.hit_sky=false;
                primaryRayPayload.payload.color = vec3(0.0);
                vec3 diffuseVector;
                if (bool(frame.use_blue_noise))
                {
                    diffuseVector = RandomDiffuseVectorFromNoise(to_light_dir);
                }
                else
                {
                    diffuseVector = RandomDiffuseVectorFromRNG(to_light_dir);
                }
                newDir = RandomSpecularVector(to_light_dir, diffuseVector, LIGHT_BIAS);
                traceLight(origin, newDir, 1000.0);;

                if (primaryRayPayload.payload.hit_sky)
                {
                    color += primaryRayPayload.payload.color;//*dot(hitResult.normal, newDir);
                    num_sun_samples++;
                }
            }

            primaryRayPayload.payload.hit_sky=false;

            if (bool(frame.use_blue_noise))
            {
                newDir = RandomDiffuseVectorFromNoise(normal);
            }
            else
            {
                newDir = RandomDiffuseVectorFromRNG(normal);
            }

            if (material.roughness<0.999)
            {
                newDir = RandomSpecularVector(reflect(gl_WorldRayDirectionEXT, normal), newDir, material.roughness);
            }
            trace(origin, newDir, 1000.0);
            color += primaryRayPayload.payload.color*dot(normal, newDir);

        }
        color /= float(numSamples+num_sun_samples);
    }

    vec4 materialColor = material.albedo;
    if (material.has_albedo==1)
    {
        color *= texture(textures[nonuniformEXT(material.albedo_index)], uvs).rgb;
    }


    primaryRayPayload.payload.color = (material.emission.rgb) + (color*materialColor.rgb);
    primaryRayPayload.payload.hit_normal = normal;
    primaryRayPayload.payload.hit_t = gl_HitTEXT;
    primaryRayPayload.payload.bounce_count = primaryRayPayload.payload.bounce_count-1;
}
