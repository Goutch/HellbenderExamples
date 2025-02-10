
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
	uint use_blue_noise = 1;
};

struct GBufferResources {
	std::vector<CameraProperties> history_camera;
	//raytracer
	std::vector<Image *> history_albedo;
	std::vector<Image *> history_normal_depth;
	std::vector<Image *> history_motion;
	std::vector<Image *> history_irradiance;
	std::vector<Image *> history_position;
	//denoiser
	Image *denoiser_temporal_accumulation_texture = nullptr;
	Image *denoiser_irradiance_vertical_blur_texture = nullptr;
};

struct RaytracerResources {
	RaytracingPipelineInstance *pipeline_instance;
	RaytracingPipeline *pipeline;
	Shader *raygen_shader;
	std::vector<Shader *> miss_shaders;
	std::vector<Shader *> hit_shaders;
	std::vector<RaytracingShaderGroup> shader_groups;
	std::vector<Image *> st_blue_noise;
};

struct SceneResources {
	RootAccelerationStructure *root_acceleration_structure;
	AABBAccelerationStructure *aabb_acceleration_structure;
	std::vector<AccelerationStructureInstance> acceleration_structure_instances;
	std::vector<InstanceInfo> instances;
	StorageBuffer *instance_buffer;

	std::vector<MaterialData> materials;
	StorageBuffer *material_buffer;

	//deleted in model parser
	std::vector<Mesh *> meshes;
	std::vector<StorageBuffer *> normals;
	std::vector<StorageBuffer *> indices;
	std::vector<StorageBuffer *> uvs;
	std::vector<Image *> textures;
	std::vector<MeshAccelerationStructure *> mesh_acceleration_structures;
};

struct DenoisingResources {
	Shader *temporal_accumulation_shader = nullptr;
	ComputePipeline *temporal_accumulation_pipeline = nullptr;
	ComputeInstance *temporal_accumulation_instance = nullptr;

	Shader *vertical_blur_shader = nullptr;
	ComputePipeline *vertical_blur_pipeline = nullptr;
	ComputeInstance *vertical_blur_instance = nullptr;

	Shader *horizontal_blur_shader = nullptr;
	ComputePipeline *horizontal_blur_pipeline = nullptr;
	ComputeInstance *horizontal_blur_instance = nullptr;

};