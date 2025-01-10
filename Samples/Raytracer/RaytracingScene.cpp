
#include "RaytracingScene.h"

void RaytracingScene::createRaytracingPipeline() {
	ShaderInfo shader_info{};

	shader_info.stage = SHADER_STAGE_RAY_GEN;
	shader_info.path = "shaders/raytracing/raygen/raygen.glsl";
	raytracing_resources.raygen_shader = Resources::createShader(shader_info);

	shader_info.stage = SHADER_STAGE_RAY_MISS;
	shader_info.path = "shaders/raytracing/miss/miss.glsl";
	raytracing_resources.miss_shaders.push_back(Resources::createShader(shader_info));

	shader_info.stage = SHADER_STAGE_CLOSEST_HIT;
	shader_info.path = "shaders/raytracing/closestHit/closest_hit.glsl";
	raytracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));
	shader_info.path = "shaders/raytracing/closestHit/closest_hit_mesh.glsl";
	raytracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));

	shader_info.stage = SHADER_STAGE_INTERSECTION;
	shader_info.path = "shaders/raytracing/intersect/intersect_box.glsl";
	raytracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));
	shader_info.path = "shaders/raytracing/intersect/intersect_sphere.glsl";
	raytracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));

	raytracing_resources.shader_groups.push_back({0, -1, 2});//box
	raytracing_resources.shader_groups.push_back({0, -1, 3});//sphere
	raytracing_resources.shader_groups.push_back({1, -1, -1});//Mesh

	RaytracingPipelineInfo raytraycing_pipeline_info{};

	raytraycing_pipeline_info.raygen_shader = raytracing_resources.raygen_shader;
	raytraycing_pipeline_info.miss_shaders = raytracing_resources.miss_shaders.data();
	raytraycing_pipeline_info.miss_shader_count = raytracing_resources.miss_shaders.size();
	raytraycing_pipeline_info.hit_shaders = raytracing_resources.hit_shaders.data();
	raytraycing_pipeline_info.hit_shader_count = raytracing_resources.hit_shaders.size();

	raytraycing_pipeline_info.shader_group_count = raytracing_resources.shader_groups.size();
	raytraycing_pipeline_info.shader_groups = raytracing_resources.shader_groups.data();
	raytraycing_pipeline_info.max_recursion_depth = 16;

	raytracing_resources.pipeline = Resources::createRaytracingPipeline(raytraycing_pipeline_info);
}

RaytracingScene::RaytracingScene() {

	blue_noise = Texture::load("/textures/BlueNoiseRGB.png", IMAGE_FORMAT_RGBA8_UINT, IMAGE_FLAG_NO_SAMPLER);
	createFrameBuffers(Graphics::getDefaultRenderTarget()->getResolution().x, Graphics::getDefaultRenderTarget()->getResolution().y);
	createRaytracingPipeline();
	RaytracingModelParserInfo model_parser_info{};
	model_parser_info.mesh_shader_group_index = SHADER_GROUP_TYPE_MESH;
	model_parser_info.acceleration_structures = &mesh_acceleration_structures;
	model_parser_info.materials = &materials;
	model_parser_info.textures = &textures;
	model_parser_info.meshes = &meshes;

	model_parser = new RaytracingModelParser(model_parser_info);

	ModelInfo model_info{};
	model_info.flags = MODEL_FLAG_USED_IN_RAYTRACING;
	model_info.parser = model_parser;
	model_info.path = "/models/sponza/Sponza.gltf";

	sponza_model = Resources::createModel(model_info);

	AABBAccelerationStructureInfo aabb__acceleration_structure_info{};
	aabb__acceleration_structure_info.max = vec3(0.5, 0.5, 0.5);
	aabb__acceleration_structure_info.min = vec3(-0.5, -0.5, -0.5);

	aabb_acceleration_structure = Resources::createAABBAccelerationStructure(aabb__acceleration_structure_info);

	std::vector<AABBAccelerationStructure *> aabb_acceleration_structures{aabb_acceleration_structure};

	mat4 transform_aabb_floor(1.0f);
	transform_aabb_floor = glm::translate(transform_aabb_floor, vec3(0, -0.5, 0));
	transform_aabb_floor = glm::scale(transform_aabb_floor, vec3(1000, 1, 1000));
	std::vector<AccelerationStructureInstance> sponza_acceleration_structure_instances = sponza_model->getAccelerationStructureInstances();
	for (AccelerationStructureInstance &instance: sponza_acceleration_structure_instances) {
		instance.transform = glm::translate(instance.transform, vec3(0, 10.0f, 0));
	}
	acceleration_structure_instances.insert(acceleration_structure_instances.end(), sponza_acceleration_structure_instances.begin(), sponza_acceleration_structure_instances.end());
	acceleration_structure_instances.push_back(AccelerationStructureInstance{0, SHADER_GROUP_TYPE_BOX, transform_aabb_floor, ACCELERATION_STRUCTURE_TYPE_AABB, 0});
	//createSphereField(200);
	RootAccelerationStructureInfo root_acceleration_structure_info{};
	root_acceleration_structure_info.aabb_acceleration_structures = aabb_acceleration_structures.data();
	root_acceleration_structure_info.aabb_acceleration_structure_count = aabb_acceleration_structures.size();
	root_acceleration_structure_info.mesh_acceleration_structures = mesh_acceleration_structures.data();
	root_acceleration_structure_info.mesh_acceleration_structure_count = mesh_acceleration_structures.size();
	root_acceleration_structure_info.instances = acceleration_structure_instances.data();
	root_acceleration_structure_info.instance_count = acceleration_structure_instances.size();

	root_acceleration_structure = Resources::createRootAccelerationStructure(root_acceleration_structure_info);

	StorageBufferInfo material_storage_buffer_info{};
	material_storage_buffer_info.stride = sizeof(MaterialData);
	material_storage_buffer_info.count = materials.size();
	material_storage_buffer_info.flags = STORAGE_BUFFER_FLAG_NONE;
	material_buffer = Resources::createStorageBuffer(material_storage_buffer_info);
	material_buffer->update(materials.data());

	std::vector<InstanceInfo> instance_infos;
	for (uint32_t i = 0; i < acceleration_structure_instances.size(); ++i) {
		instance_infos.push_back({acceleration_structure_instances[i].custom_index, i, 16});
	}
	StorageBufferInfo instances_storage_buffer_info{};
	instances_storage_buffer_info.stride = sizeof(InstanceInfo);
	instances_storage_buffer_info.count = instance_infos.size();
	instances_storage_buffer_info.flags = STORAGE_BUFFER_FLAG_NONE;
	instance_buffer = Resources::createStorageBuffer(instances_storage_buffer_info);
	instance_buffer->update(instance_infos.data());

	std::vector<StorageBuffer *> normals;
	std::vector<StorageBuffer *> indices;
	std::vector<StorageBuffer *> uvs;
	for (uint32_t i = 0; i < meshes.size(); i++) {
		normals.push_back(meshes[i]->getAttributeStorageBuffer(2));
		indices.push_back(meshes[i]->getIndicesStorageBuffer());
		uvs.push_back(meshes[i]->getAttributeStorageBuffer(1));
	}

	RaytracingPipelineInstanceInfo pathtracing_pipeline_instance_info{};
	pathtracing_pipeline_instance_info.raytracing_pipeline = raytracing_resources.pipeline;
	raytracing_resources.pipeline_instance = Resources::createRaytracingPipelineInstance(pathtracing_pipeline_instance_info);
	raytracing_resources.pipeline_instance->setTextureArray("historyAlbedo", history_albedo.data(), HYSTORY_COUNT, -1, 0);
	raytracing_resources.pipeline_instance->setTextureArray("historyNormalDepth", history_normal_depth.data(), HYSTORY_COUNT, -1, 0);
	raytracing_resources.pipeline_instance->setTextureArray("historyMotion", history_motion.data(), HYSTORY_COUNT, -1, 0);
	if (textures.size() > 0)
		raytracing_resources.pipeline_instance->setTextureArray("textures", textures.data(), textures.size());
	raytracing_resources.pipeline_instance->setStorageBuffer("materials", material_buffer, material_buffer->getCount(), 0);
	raytracing_resources.pipeline_instance->setStorageBuffer("instances", instance_buffer, instance_buffer->getCount(), 0);
	raytracing_resources.pipeline_instance->setAccelerationStructure("topLevelAS", root_acceleration_structure);
	raytracing_resources.pipeline_instance->setStorageBufferArray("mesh_indices_buffers", indices.data(), indices.size(), -1);
	raytracing_resources.pipeline_instance->setStorageBufferArray("mesh_normals_buffers", normals.data(), normals.size(), -1);
	raytracing_resources.pipeline_instance->setStorageBufferArray("mesh_tex_coords_buffers", uvs.data(), uvs.size(), -1);


	Entity camera_entity = createEntity3D();
	camera_entity.attach<Camera>();
	camera_entity.get<Camera>()->setRenderTarget(Graphics::getDefaultRenderTarget());
	camera_entity.get<Camera>()->active = false;
	camera_entity.get<Transform>()->translate(vec3(0, 1, 0));
	Camera *camera = camera_entity.get<Camera>();
	setCameraEntity(camera_entity);
	history_camera.resize(HYSTORY_COUNT, {camera_entity.get<Transform>()->world(), camera->projection});

	Graphics::getDefaultRenderTarget()->onResolutionChange.subscribe(this, &RaytracingScene::onResolutionChange);
}

void RaytracingScene::createFrameBuffers(uint32_t width, uint32_t height) {
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
	info.data_format = IMAGE_FORMAT_RGBA32F;
	info.flags = IMAGE_FLAG_SHADER_WRITE;
	info.sampler_info.address_mode = TEXTURE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.sampler_info.filter = TEXTURE_SAMPLER_FILTER_TYPE_NEAREST;
	info.sampler_info.flags = TEXTURE_SAMPLER_FLAG_NONE;

	for (uint32_t i = 0; i < HYSTORY_COUNT; i++) {
		history_albedo.push_back(Resources::createTexture(info));
		history_normal_depth.push_back(Resources::createTexture(info));
		history_motion.push_back(Resources::createTexture(info));
	}

	output_texture = Resources::createTexture(info);
}

void RaytracingScene::onResolutionChange(RenderTarget *rt) {
	createFrameBuffers(rt->getResolution().x, rt->getResolution().y);

	raytracing_resources.pipeline_instance->setTextureArray("historyAlbedo", history_albedo.data(), HYSTORY_COUNT, -1, 0);
	raytracing_resources.pipeline_instance->setTextureArray("historyNormalDepth", history_normal_depth.data(), HYSTORY_COUNT, -1, 0);
	raytracing_resources.pipeline_instance->setTextureArray("historyMotion", history_motion.data(), HYSTORY_COUNT, -1, 0);
	Camera *camera = getCameraEntity().get<Camera>();
	camera->setRenderTarget(rt);
}

void RaytracingScene::render() {
	Scene::render();
	Entity camera_entity = getCameraEntity();
	if (!paused)
		frame.time = time;

	RaytracingPipelineResources resources = raytracing_resources;


	mat4 camera_projection = camera_entity.get<Camera>()->projection;
	mat4 camera_view = camera_entity.get<Transform>()->world();
	history_camera[frame.index % HYSTORY_COUNT] = {camera_view, camera_projection};

	resources.pipeline_instance->setUniform("frame", &frame, Graphics::getCurrentFrame());
	resources.pipeline_instance->setTexture("outputAlbedo", output_texture, Graphics::getCurrentFrame(), 0);
	resources.pipeline_instance->setTexture("blueNoise", blue_noise, Graphics::getCurrentFrame(), 0);
	resources.pipeline_instance->setUniform("camera_history", history_camera.data(), Graphics::getCurrentFrame());


	TraceRaysCmdInfo trace_rays_cmd_info{};
	trace_rays_cmd_info.pipeline_instance = resources.pipeline_instance;
	trace_rays_cmd_info.root_acceleration_structure = root_acceleration_structure;
	trace_rays_cmd_info.resolution = output_texture->getSize();
	Graphics::traceRays(trace_rays_cmd_info);

	frame.index++;
}

Texture *RaytracingScene::getMainCameraTexture() {
	switch (render_mode) {
		case DENOISED:
			return output_texture;
			break;
		case ALBEDO:
			return history_albedo[(frame.index - 1) % HYSTORY_COUNT];
			break;
		case NORMAL:
			return history_normal_depth[(frame.index - 1) % HYSTORY_COUNT];
			break;
		case MOTION:
			return history_motion[(frame.index - 1) % HYSTORY_COUNT];
			break;
	}
}

void RaytracingScene::update(float delta) {
	Scene::update(delta);

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
	if (Input::getKey(KEY_NUMBER_7)) {
		frame.gamma -= delta;
		Log::message("Gamma:" + std::to_string(frame.gamma));
	}
	if (Input::getKey(KEY_NUMBER_8)) {
		frame.gamma += delta;
		Log::message("Gamma:" + std::to_string(frame.gamma));
	}
	if (Input::getKey(KEY_NUMBER_9)) {
		frame.exposure -= delta;
		Log::message("Exposure:" + std::to_string(frame.exposure));
	}
	if (Input::getKey(KEY_NUMBER_0)) {
		frame.exposure += delta;
		Log::message("Exposure:" + std::to_string(frame.exposure));
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

	if (!paused)
		time += delta * 0.05;
}

RaytracingScene::~RaytracingScene() {

	for (Shader *shader: raytracing_resources.miss_shaders) {
		delete shader;
	}
	for (Shader *shader: raytracing_resources.hit_shaders) {
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
	delete raytracing_resources.raygen_shader;
	//delete blue_noise;
	delete material_buffer;
	delete instance_buffer;
	delete aabb_acceleration_structure;
	delete root_acceleration_structure;
	delete raytracing_resources.pipeline_instance;
	delete raytracing_resources.pipeline;
	delete model_parser;
	delete sponza_model;
}

void RaytracingScene::createSphereField(int n) {
	for (uint32_t i = 0; i < n; ++i) {
		float radius = Random::floatRange(0.5, 2.5);
		Transform t{};
		t.translate(vec3(Random::floatRange(-50, 50), radius, Random::floatRange(-50, 50)));
		t.setLocalScale(vec3(radius * 2.0f));
		acceleration_structure_instances.push_back(
				AccelerationStructureInstance{0, SHADER_GROUP_TYPE_SPHERE, t.local(), ACCELERATION_STRUCTURE_TYPE_AABB, Random::uintRange(0, 8)});
	}
}

void RaytracingScene::createMaterialDisplay() {
	for (uint32_t i = 0; i < 10; ++i) {
		mat4 t(1.0f);
		t = glm::translate(t, vec3(2 * (i + 1), 0, 0));
		acceleration_structure_instances.push_back(
				AccelerationStructureInstance{0, SHADER_GROUP_TYPE_BOX, t, ACCELERATION_STRUCTURE_TYPE_AABB, i});
	}
	for (uint32_t i = 0; i < 7; ++i) {
		mat4 t(1.0f);
		t = glm::translate(t, vec3((2 * i) + 1, 0, 0));
		acceleration_structure_instances.push_back(
				AccelerationStructureInstance{0, SHADER_GROUP_TYPE_SPHERE, t, ACCELERATION_STRUCTURE_TYPE_AABB, i % 7});
	}
}

