//https://github.com/KhronosGroup/GLSL/blob/main/extensions/nv/GLSL_NV_ray_tracing.txt
const float LIGHT_BIAS = 0.1;

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
vec3 Perpendicular(vec3 normal) {
    return abs(normal.x) > abs(normal.z) ? vec3(-normal.y, normal.x, 0.0) : vec3(0.0, -normal.z, normal.y);
}
vec3 SampleCosineWeightedHemisphere(vec3 normal, inout uint state) {
    float random1 = RandomFloat01(state);
    float random2 = RandomFloat01(state);
    // Step 1: Convert random inputs to spherical coordinates
    float elevationAngle = acos(sqrt(1.0 - random1));// Elevation (theta), cosine-weighted
    float azimuthAngle = 2.0 * PI * random2;// Azimuth (phi), uniformly distributed

    // Step 2: Convert spherical coordinates to Cartesian (local tangent space)
    float x = cos(azimuthAngle) * sin(elevationAngle);
    float y = sin(azimuthAngle) * sin(elevationAngle);
    float z = cos(elevationAngle);

    // Step 3: Build an orthonormal basis (Tangent-Bitangent-Normal)
    vec3 tangent = normalize(Perpendicular(normal));// A vector perpendicular to the normal
    vec3 bitangent = cross(normal, tangent);// Bitangent completes the basis

    // Step 4: Transform local sample direction to world space
    vec3 worldDirection = x * tangent + y * bitangent + z * normal;

    return worldDirection;
}
vec3 BlueNoiseDiffuseVector(vec3 normal)
{
    //https://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
    vec2 uvOffset = mod(vec2(0.5+A1*payload.rng_state, 0.5+A2*payload.rng_state), vec2(1.0));

    ivec2 offset = ivec2(floor(uvOffset * 128.0));
    ivec3 sample_coord = ivec3(mod(gl_LaunchIDEXT.x + offset.x, 128), mod(gl_LaunchIDEXT.y+offset.y, 128), mod(frame.data.index, 64));

    vec3 N = normalize(normal);
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);
    vec3 tagent_diffuse = imageLoad(blue_noise[sample_coord.z], sample_coord.xy).xyz;
    tagent_diffuse = tagent_diffuse * 2.0 - 1.0;
    vec3 world_space_diffuse = TBN * tagent_diffuse.xyz;

    payload.rng_state++;

    return world_space_diffuse;
}
vec3 WhiteNoiseDiffuseVector(vec3 normal)
{
    return normalize(SampleCosineWeightedHemisphere(normal, payload.rng_state));
}
vec3 RandomSpecularVector(vec3 direction, vec3 diffuse, float roughness)
{
    return normalize(mix(direction, diffuse, roughness*roughness));
}

void traceLight(vec3 origin, vec3 dir, float tmax)
{
    traceRayEXT(topLevelAS, //topLevelacceleationStructure
    gl_RayFlagsOpaqueEXT|gl_RayFlagsSkipClosestHitShaderEXT|  gl_RayFlagsTerminateOnFirstHitEXT, //rayFlags
    0xFF, //cullMask
    0, //sbtRecordOffset
    1, //sbtRecordStride
    0, //missIndex
    origin, //origin
    T_MIN, //Tmin
    dir, //direction
    tmax, //Tmax
    0);//payload location
}
void trace(vec3 origin, vec3 dir, float tmax)
{
    payload.irradiance=vec3(0.0);
    traceRayEXT(topLevelAS, //topLevelacceleationStructure
    gl_RayFlagsOpaqueEXT, //rayFlags
    0xFF, //cullMask
    0, //sbtRecordOffset
    1, //sbtRecordStride
    0, //missIndex
    origin, //origin
    T_MIN, //Tmin
    dir, //direction
    tmax, //Tmax
    0);//payload location
}
vec3 traceSecondaryRay(inout vec3 incomingRayDir, vec3 position, MaterialData material, vec3 normal, vec3 albedo, bool trace_light)
{
    vec3 to_light_dir= normalize(vec3(sin(frame.data.time), sin(frame.data.time), cos(frame.data.time)));

    vec3 light_irradiance =vec3(0.0);
    vec3 gi_irradiance =vec3(0.0);
    int num_sun_samples = 0;

    vec3 new_dir;
    if (trace_light)
    {
        //float light_dot_product=dot(normal, to_light_dir);
        //if (light_dot_product>0.0 && material.roughness>0.999 && smoothstep(0, 1, light_dot_product)>RandomFloat01(payload.rng_state))
        //{
        payload.hit_sky=false;
        payload.irradiance = vec3(0.0);

        if (frame.data.use_blue_noise!=0)
        {
            new_dir = BlueNoiseDiffuseVector(to_light_dir);
        }
        else
        {
            new_dir = WhiteNoiseDiffuseVector(to_light_dir);
        }
        new_dir = RandomSpecularVector(to_light_dir, new_dir, LIGHT_BIAS);
        traceLight(position, new_dir, 1000.0);;
        //}
    }
    if (payload.hit_sky)
    {
        light_irradiance += payload.irradiance * dot(normal, to_light_dir);
        num_sun_samples++;
    }
    else
    {
        payload.hit_sky=false;

        if (frame.data.use_blue_noise!=0)
        {
            new_dir = BlueNoiseDiffuseVector(normal);
        }
        else
        {
            new_dir = WhiteNoiseDiffuseVector(normal);
        }
        if (material.roughness<0.999)
        {
            new_dir = RandomSpecularVector(reflect(incomingRayDir, normal), new_dir, material.roughness);
        }

        trace(position, new_dir, 1000.0);

        gi_irradiance += payload.irradiance*dot(normal, new_dir);
    }

    vec3 irradiance = light_irradiance+gi_irradiance;

    incomingRayDir = new_dir;
    return (material.emission.rgb) + (albedo * irradiance);
}

