
#pragma once

#include "HBE.h"

struct MaterialData {
	vec4 albedo;
	vec4 emission;
	int albedo_texture_index = -1;
	int normal_texture_index = -1;
	alignas(8) float roughness;
};
struct InstanceInfo {
	uint32_t material_index;
	uint32_t mesh_index;
	uint32_t indices_size;
};

struct CameraProperties {
	mat4 transform;
	mat4 projection;
};

struct Frame {
	float time = 0;
	uint32_t index = 0;
	uint32_t sample_count = 1;
	uint32_t max_bounces = 3;
	float scattering_multiplier = 12.0f;
	float density_falloff = 10.0f;
	float exposure = 1.0f;
	float gamma = 2.2f;
};