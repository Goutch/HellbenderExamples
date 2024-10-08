#pragma once

#include "HBE.h"
#include "vector"
#include "array"
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

struct MaterialData {
	vec4 albedo;
	vec4 emission;
	alignas(16) float roughness;
};

class RaytracingScene : public Scene {

private:
	enum RENDER_MODE {
		DENOISED = 0,
		ALBEDO = 1,
		NORMAL = 2,
		MOTION = 3,
	};
	RaytracingPipelineResources pathtracing_resources;
	std::vector<Shader *> common_hit_shaders;
	RootAccelerationStructure *root_acceleration_structure;
	AABBAccelerationStructure *aabb_acceleration_structure;
	AABBAccelerationStructure *mesh_acceleration_structure;
	StorageBuffer *material_buffer;
	const std::array<MaterialData, 9> materials = {{
			                                               {vec4(0.95, 0.95, 0.95, 0.0), vec4(0.0, 0.0, 0.0, 0.0), 1.0},//white
			                                               {vec4(0.5, 0.5, 0.5, 0.0), vec4(10.0, 10.0, 10.0, 10.0), 1.0},//light
			                                               {vec4(0.95, 0, 0, 0.0), vec4(0.0, 0.0, 0.0, 0.0), 1.0},//red
			                                               {vec4(0, 0.95, 0, 0.0), vec4(0.0, 0.0, 0.0, 0.0), 1.0},//green
			                                               {vec4(0, 0, 0.95, 0.0), vec4(0.0, 0.0, 0.0, 0.0), 1.0},//blue
			                                               {vec4(0.9, 0.9, 0.1, 0.0), vec4(0.0, 0.0, 0.0, 0.0), 1.0},//yellow
			                                               {vec4(0.9, 0.1, 0.9, 0.0), vec4(0.0, 0.0, 0.0, 0.0), 1.0},//purple
			                                               {vec4(0.2, 0.2, 0.2, 0.0), vec4(0.0, 0.0, 0.0, 0.0), 0.4},//metalic
			                                               {vec4(0.8, 0.8, 0.8, 0.0), vec4(0.0, 0.0, 0.0, 0.0), 0.0}}//mirror
	};
	enum MATERIALS {
		MATERIAL_WHITE = 0,
		MATERIAL_LIGHT = 1,
		MATERIAL_RED = 2,
		MATERIAL_GREEN = 3,
		MATERIAL_BLUE = 4,
		MATERIAL_YELLOW = 5,
		MATERIAL_PURPLE = 6,
		MATERIAL_METALIC = 7,
		MATERIAL_MIRROR = 8,
	};
	std::vector<Texture *> history_albedo;
	std::vector<Texture *> history_normal_depth;
	std::vector<Texture *> history_motion;

	Texture *last_albedo;
	Texture *last_normal_depth;
	Texture *last_motion;

	Texture *blue_noise;
	Texture *output_texture = nullptr;
	Frame frame{};
	bool paused = false;
	RENDER_MODE render_mode = DENOISED;

	mat4 last_camera_matrices[2] = {mat4(1.0), mat4(1.0)};
public:

	void createFrameBuffers(uint32_t width, uint32_t height) {
		for (Texture *albedo: history_albedo) {
			delete albedo;
		}
		for (Texture *normal_depth: history_normal_depth) {
			delete normal_depth;
		}
		for (Texture *motion: history_motion) {
			delete motion;
		}
		if (output_texture != nullptr) {
			delete output_texture;
		}
		history_albedo.clear();
		history_normal_depth.clear();
		history_motion.clear();

		TextureInfo info{};
		info.width = width;
		info.height = height;
		info.format = IMAGE_FORMAT_RGBA32F;
		info.flags = IMAGE_FLAG_SHADER_WRITE |
		             IMAGE_FLAG_FILTER_NEAREST;

		for (uint32_t i = 0; i < HYSTORY_COUNT; i++) {
			history_albedo.push_back(Resources::createTexture(info));
			history_normal_depth.push_back(Resources::createTexture(info));
			history_motion.push_back(Resources::createTexture(info));
		}

		output_texture = Resources::createTexture(info);
	}

	void onResolutionChange(RenderTarget *rt) {
		createFrameBuffers(rt->getResolution().x, rt->getResolution().y);

		Camera *camera = getCameraEntity().get<Camera>();
		camera->setRenderTarget(rt);
	}


	void render() override {
		Scene::render();
		Entity camera_entity = getCameraEntity();
		if (!paused)
			frame.time = Application::getTime() * 0.05;

		RaytracingPipelineResources resources = pathtracing_resources;


		Texture *albedo_history_buffer[HYSTORY_COUNT];
		Texture *normal_depth_history_buffer[HYSTORY_COUNT];
		Texture *motion_history_buffer[HYSTORY_COUNT];

		for (uint32_t i = 0; i < HYSTORY_COUNT; i++) {
			uint32_t f = (frame.index - i) % HYSTORY_COUNT;
			albedo_history_buffer[i] = history_albedo[f];
			normal_depth_history_buffer[i] = history_normal_depth[f];
			motion_history_buffer[i] = history_motion[f];
		}

		mat4 camera_projection = camera_entity.get<Camera>()->projection;
		mat4 camera_view = camera_entity.get<Transform>()->world();

		resources.pipeline_instance->setUniform("frame", &frame, Graphics::getCurrentFrame());
		resources.pipeline_instance->setTexture("outputAlbedo", output_texture, Graphics::getCurrentFrame(), 0);
		resources.pipeline_instance->setTextureArray("historyAlbedo", albedo_history_buffer, HYSTORY_COUNT, Graphics::getCurrentFrame(), 0);
		resources.pipeline_instance->setTextureArray("historyNormalDepth", normal_depth_history_buffer, HYSTORY_COUNT, Graphics::getCurrentFrame(), 0);
		resources.pipeline_instance->setTextureArray("historyMotion", motion_history_buffer, HYSTORY_COUNT, Graphics::getCurrentFrame(), 0);
		resources.pipeline_instance->setTexture("blueNoise", blue_noise, Graphics::getCurrentFrame(), 0);
		resources.pipeline_instance->setUniform("last_cam", &last_camera_matrices, Graphics::getCurrentFrame());
		mat4 camera_ubo[2] = {camera_view, glm::inverse(camera_projection)};
		resources.pipeline_instance->setUniform("cam", &camera_ubo, Graphics::getCurrentFrame());


		TraceRaysCmdInfo trace_rays_cmd_info{};
		trace_rays_cmd_info.pipeline_instance = resources.pipeline_instance;
		trace_rays_cmd_info.root_acceleration_structure = root_acceleration_structure;
		trace_rays_cmd_info.resolution = output_texture->getSize();
		Graphics::traceRays(trace_rays_cmd_info);

		last_camera_matrices[0] = glm::inverse(camera_view);
		last_camera_matrices[1] = camera_projection;


		last_albedo = albedo_history_buffer[0];
		last_normal_depth = normal_depth_history_buffer[0];
		last_motion = motion_history_buffer[0];

		frame.index++;
	}

	Texture *getMainCameraTexture() override {
		switch (render_mode) {
			case DENOISED:
				return output_texture;
				break;
			case ALBEDO:
				return last_albedo;
				break;
			case NORMAL:
				return last_normal_depth;
				break;
			case MOTION:
				return last_motion;
				break;
		}
	}

	void update(float delta) override {
		Scene::update(delta);
		if (Input::getKeyDown(KEY_NUMBER_0)) {
			frame.use_blue_noise = !bool(frame.use_blue_noise);
		}
		if (Input::getKeyDown(KEY_NUMBER_1)) {
			render_mode = DENOISED;
		}
		if (Input::getKeyDown(KEY_NUMBER_2)) {
			render_mode = ALBEDO;
		}
		if (Input::getKeyDown(KEY_NUMBER_3)) {
			render_mode = NORMAL;
		}
		if (Input::getKeyDown(KEY_NUMBER_4)) {
			render_mode = MOTION;
		}
		if (Input::getKeyDown(KEY_P)) {
			paused = !paused;
		}
		if (Input::getKey(KEY_MINUS)) {
			frame.scattering_multiplier -= 5.0f * delta;
			Log::message("Scattering multiplier:" + std::to_string(frame.scattering_multiplier));
		}
		if (Input::getKey(KEY_EQUAL)) {
			frame.scattering_multiplier += 5.0f * delta;

			Log::message("Scattering multiplier:" + std::to_string(frame.scattering_multiplier));
		}
		if (Input::getKey(KEY_LEFT_BRACKET)) {
			frame.density_falloff -= 5.0f * delta;
			Log::message("Density falloff:" + std::to_string(frame.density_falloff));
		}
		if (Input::getKey(KEY_RIGHT_BRACKET)) {
			frame.density_falloff += 5.0f * delta;
			Log::message("Density falloff:" + std::to_string(frame.density_falloff));
		}
		if (Input::getKeyDown(KEY_R)) {
			frame.sample_count = 1;
		}
		if (Input::getKeyDown(KEY_UP)) {
			frame.sample_count++;
			Log::message("Sample count:" + std::to_string(frame.sample_count));
		}
		if (Input::getKeyDown(KEY_DOWN)) {
			if (frame.sample_count > 0) {
				frame.sample_count--;
			}
			Log::message("Sample count:" + std::to_string(frame.sample_count));
		}
		if (Input::getKeyDown(KEY_RIGHT)) {
			frame.max_bounces++;
			Log::message("Max bounces:" + std::to_string(frame.max_bounces));
		}
		if (Input::getKeyDown(KEY_LEFT)) {
			if (frame.max_bounces > 0) {
				frame.max_bounces--;
			}
			Log::message("Max bounces:" + std::to_string(frame.max_bounces));
		}
	}

	~RaytracingScene() {

		for (Shader *shader: pathtracing_resources.miss_shaders) {
			delete shader;
		}
		for (Shader *shader: pathtracing_resources.hit_shaders) {
			delete shader;
		}
		for (Texture *albedo: history_albedo) {
			delete albedo;
		}
		for (Texture *normal_depth: history_normal_depth) {
			delete normal_depth;
		}
		for (Texture *motion: history_motion) {
			delete motion;
		}
		if (output_texture != nullptr) {
			delete output_texture;
		}
		history_albedo.clear();
		history_normal_depth.clear();
		history_motion.clear();
		delete pathtracing_resources.raygen_shader;
		//delete blue_noise;
		delete material_buffer;
		//delete mesh_acceleration_structure;
		delete aabb_acceleration_structure;
		delete root_acceleration_structure;
		delete pathtracing_resources.pipeline_instance;
		delete pathtracing_resources.pipeline;

	}


	RaytracingScene() {
		createFrameBuffers(Graphics::getDefaultRenderTarget()->getResolution().x, Graphics::getDefaultRenderTarget()->getResolution().y);

		blue_noise = Texture::load("/textures/BlueNoiseRGB.png", IMAGE_FORMAT_RGBA8, IMAGE_FLAG_FILTER_NEAREST | IMAGE_FLAG_NO_SAMPLER);

		ShaderInfo shader_info{};

		shader_info.stage = SHADER_STAGE_RAY_GEN;
		shader_info.path = "shaders/raygen_pathtrace.glsl";
		pathtracing_resources.raygen_shader = Resources::createShader(shader_info);


		shader_info.stage = SHADER_STAGE_RAY_MISS;
		shader_info.path = "shaders/miss_pathtrace.glsl";
		pathtracing_resources.miss_shaders.push_back(Resources::createShader(shader_info));


		shader_info.stage = SHADER_STAGE_CLOSEST_HIT;
		shader_info.path = "shaders/closesthit_aabb.glsl";
		pathtracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));


		shader_info.stage = SHADER_STAGE_INTERSECTION;
		shader_info.path = "shaders/intersect_box.glsl";
		pathtracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));
		shader_info.path = "shaders/intersect_sphere.glsl";
		pathtracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));

		pathtracing_resources.shader_groups.push_back({0, -1, 1});//box
		pathtracing_resources.shader_groups.push_back({0, -1, 2});//sphere

		RaytracingPipelineInfo pathtracing_pipeline_info{};

		pathtracing_pipeline_info.raygen_shader = pathtracing_resources.raygen_shader;
		pathtracing_pipeline_info.miss_shaders = pathtracing_resources.miss_shaders.data();
		pathtracing_pipeline_info.miss_shader_count = pathtracing_resources.miss_shaders.size();
		pathtracing_pipeline_info.hit_shaders = pathtracing_resources.hit_shaders.data();
		pathtracing_pipeline_info.hit_shader_count = pathtracing_resources.hit_shaders.size();

		pathtracing_pipeline_info.shader_group_count = pathtracing_resources.shader_groups.size();
		pathtracing_pipeline_info.shader_groups = pathtracing_resources.shader_groups.data();
		pathtracing_pipeline_info.max_recursion_depth = 16;

		pathtracing_resources.pipeline = Resources::createRaytracingPipeline(pathtracing_pipeline_info);

		AABBAccelerationStructureInfo aabb__acceleration_structure_info{};
		aabb__acceleration_structure_info.max = vec3(0.5, 0.5, 0.5);
		aabb__acceleration_structure_info.min = vec3(-0.5, -0.5, -0.5);

		aabb_acceleration_structure = Resources::createAABBAccelerationStructure(aabb__acceleration_structure_info);

		std::vector<AABBAccelerationStructure *> aabb_acceleration_structures{aabb_acceleration_structure};

		std::vector<AccelerationStructureInstance> acceleration_structure_instances{};

		mat4 transform_aabb_floor(1.0f);
		transform_aabb_floor = glm::translate(transform_aabb_floor, vec3(0, -1, 0));
		transform_aabb_floor = glm::scale(transform_aabb_floor, vec3(100, 1, 100));
		acceleration_structure_instances.push_back(AccelerationStructureInstance{0, 0, transform_aabb_floor, ACCELERATION_STRUCTURE_TYPE_AABB, 0});


		for (uint32_t i = 0; i < 7; ++i) {
			mat4 t(1.0f);
			t = glm::translate(t, vec3(2 * (i + 1), 0, 0));
			acceleration_structure_instances.push_back(
					AccelerationStructureInstance{0, 0, t, ACCELERATION_STRUCTURE_TYPE_AABB, i});
		}
		for (uint32_t i = 0; i < 7; ++i) {
			mat4 t(1.0f);
			t = glm::translate(t, vec3((2 * i) + 1, 0, 0));
			acceleration_structure_instances.push_back(
					AccelerationStructureInstance{0, 1, t, ACCELERATION_STRUCTURE_TYPE_AABB, i % 7});
		}

		mat4 t(1.0f);
		t = glm::translate(t, vec3(10, 2.5, 7));
		t = glm::scale(t, vec3(5, 5, 1));
		acceleration_structure_instances.push_back(AccelerationStructureInstance{0,
		                                                                         0,
		                                                                         t,
		                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
		                                                                         MATERIAL_GREEN});
		t = mat4(1.0f);
		t = glm::translate(t, vec3(10, 2.5, 13));
		t = glm::scale(t, vec3(5, 5, 1));
		acceleration_structure_instances.push_back(AccelerationStructureInstance{0,
		                                                                         0,
		                                                                         t,
		                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
		                                                                         MATERIAL_RED});
		t = mat4(1.0f);
		t = glm::translate(t, vec3(7, 2.5, 10));
		t = glm::scale(t, vec3(1, 5, 5));
		acceleration_structure_instances.push_back(AccelerationStructureInstance{0,
		                                                                         0,
		                                                                         t,
		                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
		                                                                         MATERIAL_WHITE});
		t = mat4(1.0f);
		t = glm::translate(t, vec3(13, 2.5, 10));
		t = glm::scale(t, vec3(1, 5, 5));
		acceleration_structure_instances.push_back(AccelerationStructureInstance{0,
		                                                                         0,
		                                                                         t,
		                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
		                                                                         MATERIAL_WHITE});

		//floor
		t = mat4(1.0f);
		t = glm::translate(t, vec3(10, -0.5, 10));
		t = glm::scale(t, vec3(5, 1, 5));
		acceleration_structure_instances.push_back(AccelerationStructureInstance{0,
		                                                                         0,
		                                                                         t,
		                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
		                                                                         MATERIAL_WHITE});
		//ceiling
		t = mat4(1.0f);
		t = glm::translate(t, vec3(10, 5.5, 10));
		t = glm::scale(t, vec3(5, 1, 5));
		acceleration_structure_instances.push_back(AccelerationStructureInstance{0,
		                                                                         0,
		                                                                         t,
		                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
		                                                                         MATERIAL_WHITE});

		//Ceiling light
		t = mat4(1.0f);
		t = glm::translate(t, vec3(10, 5.4, 10));
		t = glm::scale(t, vec3(2, 1, 2));
		acceleration_structure_instances.push_back(AccelerationStructureInstance{0,
		                                                                         0,
		                                                                         t,
		                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
		                                                                         MATERIAL_LIGHT});

		//sphere
		t = mat4(1.0f);
		t = glm::translate(t, vec3(10, 1, 10));
		t = glm::scale(t, vec3(1, 1, 1));
		acceleration_structure_instances.push_back(AccelerationStructureInstance{0,
		                                                                         1,
		                                                                         t,
		                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
		                                                                         MATERIAL_WHITE});

		RootAccelerationStructureInfo root_acceleration_structure_info{};
		root_acceleration_structure_info.aabb_acceleration_structures = aabb_acceleration_structures.data();
		root_acceleration_structure_info.aabb_acceleration_structure_count = aabb_acceleration_structures.size();
		//root_acceleration_structure_info.mesh_acceleration_structures = &mesh_acceleration_structure;
		//root_acceleration_structure_info.mesh_acceleration_structure_count = 1;
		root_acceleration_structure_info.instances = acceleration_structure_instances.data();
		root_acceleration_structure_info.instance_count = acceleration_structure_instances.size();

		root_acceleration_structure = Resources::createRootAccelerationStructure(root_acceleration_structure_info);

		StorageBufferInfo storage_buffer_info{};
		storage_buffer_info.stride = sizeof(MaterialData);
		storage_buffer_info.count = materials.size();
		storage_buffer_info.flags = STORAGE_BUFFER_FLAG_NONE;
		material_buffer = Resources::createStorageBuffer(storage_buffer_info);
		material_buffer->update(materials.data());

		RaytracingPipelineInstanceInfo pathtracing_pipeline_instance_info{};
		pathtracing_pipeline_instance_info.raytracing_pipeline = pathtracing_resources.pipeline;
		pathtracing_resources.pipeline_instance = Resources::createRaytracingPipelineInstance(pathtracing_pipeline_instance_info);
		pathtracing_resources.pipeline_instance->setAccelerationStructure("topLevelAS", root_acceleration_structure);
		pathtracing_resources.pipeline_instance->setStorageBuffer("materials", material_buffer, material_buffer->getCount(), 0);

		Entity camera_entity = createEntity3D();
		camera_entity.attach<Camera>();
		camera_entity.get<Camera>()->setRenderTarget(Graphics::getDefaultRenderTarget());
		camera_entity.get<Camera>()->active = false;
		camera_entity.attach<CameraController>()->invert_y = true;
		setCameraEntity(camera_entity);

		Graphics::getDefaultRenderTarget()->onResolutionChange.subscribe(this, &RaytracingScene::onResolutionChange);
	}
};


