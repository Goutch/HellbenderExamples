
#pragma once

#include "HBE.h"

struct MaterialData {
	vec4 albedo;
	vec4 emission;
	int has_albedo = 0;
	int albedo_texture_index = -1;
	alignas(8) float roughness;
};
struct InstanceInfo {
	uint32_t material_index;
	uint32_t mesh_index;
};