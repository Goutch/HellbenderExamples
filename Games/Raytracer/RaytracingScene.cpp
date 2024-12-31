
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
	acceleration_structure_instances.insert(acceleration_structure_instances.end(), sponza_model->getAccelerationStructureInstances().begin(), sponza_model->getAccelerationStructureInstances().end());
	//acceleration_structure_instances.push_back(AccelerationStructureInstance{0, SHADER_GROUP_TYPE_BOX, transform_aabb_floor, ACCELERATION_STRUCTURE_TYPE_AABB, 0});
	createSphereField(200);
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
		instance_infos.push_back({acceleration_structure_instances[i].custom_index, i});
	}
	StorageBufferInfo instances_storage_buffer_info{};
	instances_storage_buffer_info.stride = sizeof(InstanceInfo);
	instances_storage_buffer_info.count = instance_infos.size();
	instances_storage_buffer_info.flags = STORAGE_BUFFER_FLAG_NONE;
	instance_buffer = Resources::createStorageBuffer(instances_storage_buffer_info);
	instance_buffer->update(instance_infos.data());

	std::vector<StorageBuffer *> normals;
	std::vector<StorageBuffer *> indices;
	for (uint32_t i = 0; i < meshes.size(); i++) {
		normals.push_back(meshes[i]->getAttributeStorageBuffer(2));
		indices.push_back(meshes[i]->getIndicesStorageBuffer());
	}


	RaytracingPipelineInstanceInfo pathtracing_pipeline_instance_info{};
	pathtracing_pipeline_instance_info.raytracing_pipeline = raytracing_resources.pipeline;
	raytracing_resources.pipeline_instance = Resources::createRaytracingPipelineInstance(pathtracing_pipeline_instance_info);
	raytracing_resources.pipeline_instance->setAccelerationStructure("topLevelAS", root_acceleration_structure);
	raytracing_resources.pipeline_instance->setStorageBuffer("materials", material_buffer, material_buffer->getCount(), 0);
	raytracing_resources.pipeline_instance->setStorageBuffer("instances", instance_buffer, instance_buffer->getCount(), 0);
	if (textures.size() > 0)
		raytracing_resources.pipeline_instance->setTextureArray("textures", textures.data(), textures.size());
	//raytracing_resources.pipeline_instance->setStorageBufferArray("meshes_normals", normals.data(), normals.size());
	//raytracing_resources.pipeline_instance->setStorageBufferArray("meshes_indices", indices.data(), indices.size());

	Entity camera_entity = createEntity3D();
	camera_entity.attach<Camera>();
	camera_entity.get<Camera>()->setRenderTarget(Graphics::getDefaultRenderTarget());
	camera_entity.get<Camera>()->active = false;
	camera_entity.attach<CameraController>()->invert_y = true;
	camera_entity.get<Transform>()->translate(vec3(0, 1, 0));
	setCameraEntity(camera_entity);

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

	Camera *camera = getCameraEntity().get<Camera>();
	camera->setRenderTarget(rt);
}

void RaytracingScene::render() {
	Scene::render();
	Entity camera_entity = getCameraEntity();
	if (!paused)
		frame.time = Application::getTime() * 0.05;

	RaytracingPipelineResources resources = raytracing_resources;


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

Texture *RaytracingScene::getMainCameraTexture() {
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

void RaytracingScene::update(float delta) {
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

void RaytracingScene::createCornelBox() {
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
	                                                                         SHADER_GROUP_TYPE_BOX,
	                                                                         t,
	                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
	                                                                         MATERIAL_RED});
	t = mat4(1.0f);
	t = glm::translate(t, vec3(7, 2.5, 10));
	t = glm::scale(t, vec3(1, 5, 5));
	acceleration_structure_instances.push_back(AccelerationStructureInstance{0,
	                                                                         SHADER_GROUP_TYPE_BOX,
	                                                                         t,
	                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
	                                                                         MATERIAL_WHITE});
	t = mat4(1.0f);
	t = glm::translate(t, vec3(13, 2.5, 10));
	t = glm::scale(t, vec3(1, 5, 5));
	acceleration_structure_instances.push_back(AccelerationStructureInstance{0,
	                                                                         SHADER_GROUP_TYPE_BOX,
	                                                                         t,
	                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
	                                                                         MATERIAL_WHITE});

	//floor
	t = mat4(1.0f);
	t = glm::translate(t, vec3(10, -0.5, 10));
	t = glm::scale(t, vec3(5, 1, 5));
	acceleration_structure_instances.push_back(AccelerationStructureInstance{0,
	                                                                         SHADER_GROUP_TYPE_BOX,
	                                                                         t,
	                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
	                                                                         MATERIAL_WHITE});
	//ceiling
	t = mat4(1.0f);
	t = glm::translate(t, vec3(10, 5.5, 10));
	t = glm::scale(t, vec3(5, 1, 5));
	acceleration_structure_instances.push_back(AccelerationStructureInstance{0,
	                                                                         SHADER_GROUP_TYPE_BOX,
	                                                                         t,
	                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
	                                                                         MATERIAL_WHITE});

	//Ceiling light
	t = mat4(1.0f);
	t = glm::translate(t, vec3(10, 5.4, 10));
	t = glm::scale(t, vec3(2, 1, 2));
	acceleration_structure_instances.push_back(AccelerationStructureInstance{0,
	                                                                         SHADER_GROUP_TYPE_BOX,
	                                                                         t,
	                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
	                                                                         MATERIAL_LIGHT});

	//sphere
	t = mat4(1.0f);
	t = glm::translate(t, vec3(10, 1, 10));
	t = glm::scale(t, vec3(1, 1, 1));
	acceleration_structure_instances.push_back(AccelerationStructureInstance{0,
	                                                                         SHADER_GROUP_TYPE_SPHERE,
	                                                                         t,
	                                                                         ACCELERATION_STRUCTURE_TYPE_AABB,
	                                                                         MATERIAL_WHITE});
}


