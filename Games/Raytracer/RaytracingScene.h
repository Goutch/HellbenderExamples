#pragma once

#include "HBE.h"
#include "vector"
#include "array"
#include "Material.h"
#include "RaytracingModelParser.h"
//#include "NRD.h"

using namespace HBE;

struct RaytracingPipelineResources {
	RaytracingPipelineInstance *pipeline_instance;
	RaytracingPipeline *pipeline;
	Shader *raygen_shader;
	std::vector<Shader *> miss_shaders;
	std::vector<Shader *> hit_shaders;
	std::vector<RaytracingShaderGroup> shader_groups;
};

struct Frame {
	float time = 0;
	uint32_t index = 0;
	uint32_t sample_count = 1;
	uint32_t max_bounces = 3;
	float scattering_multiplier = 12.0f;
	float density_falloff = 10.0f;
	int use_blue_noise = 0;
};
#define HYSTORY_COUNT 8


class RaytracingScene : public Scene {

private:

	enum RENDER_MODE {
		DENOISED = 0,
		ALBEDO = 1,
		NORMAL = 2,
		MOTION = 3,
	};
	RaytracingPipelineResources raytracing_resources;
	std::vector<HBE::Texture *> textures;
	RootAccelerationStructure *root_acceleration_structure;
	AABBAccelerationStructure *aabb_acceleration_structure;
	std::vector<MeshAccelerationStructure *> mesh_acceleration_structures;
	std::vector<Mesh *> meshes;
	std::vector<AABBAccelerationStructure *> aabb_acceleration_structures;
	std::vector<AccelerationStructureInstance> acceleration_structure_instances;
	StorageBuffer *material_buffer;
	StorageBuffer *instance_buffer;
	RaytracingModelParser *model_parser;
	Model *sponza_model;
	std::vector<MaterialData> materials = {{
			                                       {vec4(0.95, 0.95, 0.95, 0.0), vec4(0.0, 0.0, 0.0, 0.0), -1, -1, 1.0},//white
			                                       {vec4(0.95, 0, 0, 0.0), vec4(0.0, 0.0, 0.0, 0.0), -1, -1, 1.0},//red
			                                       {vec4(0, 0.95, 0, 0.0), vec4(0.0, 0.0, 0.0, 0.0), -1, -1, 1.0},//green
			                                       {vec4(0, 0, 0.95, 0.0), vec4(0.0, 0.0, 0.0, 0.0), -1, -1, 1.0},//blue
			                                       {vec4(0.9, 0.9, 0.1, 0.0), vec4(0.0, 0.0, 0.0, 0.0), -1, -1, 1.0},//yellow
			                                       {vec4(0.9, 0.1, 0.9, 0.0), vec4(0.0, 0.0, 0.0, 0.0), -1, -1, 1.0},//purple
			                                       {vec4(0.8, 0.8, 0.8, 0.0), vec4(0.0, 0.0, 0.0, 0.0), -1, -1, 0.4},//metalic
			                                       {vec4(0.8, 0.8, 0.8, 0.0), vec4(0.0, 0.0, 0.0, 0.0), -1, -1, 0.0},//mirror
			                                       {vec4(0.5, 0.5, 0.5, 0.0), vec4(10.0, 10.0, 10.0, 10.0), -1, -1, 1.0}}//light
	};
	enum MATERIALS {
		MATERIAL_WHITE = 0,
		MATERIAL_RED = 1,
		MATERIAL_GREEN = 2,
		MATERIAL_BLUE = 3,
		MATERIAL_YELLOW = 4,
		MATERIAL_PURPLE = 5,
		MATERIAL_METALIC = 6,
		MATERIAL_MIRROR = 7,
		MATERIAL_LIGHT = 9,
	};
	enum SHADER_GROUP_TYPE {
		SHADER_GROUP_TYPE_BOX = 0,
		SHADER_GROUP_TYPE_SPHERE = 1,
		SHADER_GROUP_TYPE_MESH = 2,
	};
	std::vector<Texture *> history_albedo;
	std::vector<Texture *> history_normal_depth;
	std::vector<Texture *> history_motion;
	std::vector<CameraProperties> history_camera;

	Texture *blue_noise;
	Texture *output_texture = nullptr;
	Frame frame{};
	bool paused = false;
	RENDER_MODE render_mode = DENOISED;


public:

	void createFrameBuffers(uint32_t width, uint32_t height);

	void onResolutionChange(RenderTarget *rt);

	void createRaytracingPipeline();

	void render() override;

	Texture *getMainCameraTexture() override;

	void update(float delta) override;

	~RaytracingScene() override;

	void createSphereField(int n);

	void createMaterialDisplay();

	void createCornelBox();

	RaytracingScene();
};


