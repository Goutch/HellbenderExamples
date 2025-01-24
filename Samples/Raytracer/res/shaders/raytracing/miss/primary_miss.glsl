#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require

#define PRIMARY_PAYLOAD_IN
#include "../common.glsl"
#include "atmospheric_scattering.glsl"
void main()
{
    vec3 irradiance = calculateAtmosphericScatering();
    payload.irradiance = irradiance;
    payload.hit_sky = true;
    payload.hit_t = gl_RayTmaxEXT;
    payload.hit_normal = -gl_WorldRayDirectionEXT;
}