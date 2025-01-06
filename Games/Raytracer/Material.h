
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