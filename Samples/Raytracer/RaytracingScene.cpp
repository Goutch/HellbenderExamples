
#include "RaytracingScene.h"

void RaytracingScene::createDenoisingResources() {

	ShaderInfo shader_info{};

	shader_info.path = "shaders/denoising/vertical.comp";

	shader_info.stage = SHADER_STAGE_COMPUTE;
	shader_info.path = "shaders/denoising/horizontal.comp";
	denoising_resources.horizontal_blur_shader = Resources::createShader(shader_info);
	shader_info.path = "shaders/denoising/vertical.comp";
	denoising_resources.vertical_blur_shader = Resources::createShader(shader_info);

	ComputePipelineInfo compute_pipeline_info{};
	compute_pipeline_info.compute_shader = denoising_resources.horizontal_blur_shader;
	denoising_resources.horizontal_blur_pipeline = Resources::createComputePipeline(compute_pipeline_info);
	compute_pipeline_info.compute_shader = denoising_resources.vertical_blur_shader;
	denoising_resources.vertical_blur_pipeline = Resources::createComputePipeline(compute_pipeline_info);

	ComputeInstanceInfo compute_instance_info{};
	compute_instance_info.compute_pipeline = denoising_resources.horizontal_blur_pipeline;
	denoising_resources.horizontal_blur_instance = Resources::createComputeInstance(compute_instance_info);

	compute_instance_info.compute_pipeline = denoising_resources.vertical_blur_pipeline;
	denoising_resources.vertical_blur_instance = Resources::createComputeInstance(compute_instance_info);

	denoising_resources.vertical_blur_instance->setImageArray("historyAlbedo", gbuffer_resources.history_albedo.data(), HISTORY_COUNT);
	denoising_resources.vertical_blur_instance->setImageArray("historyNormalDepth", gbuffer_resources.history_normal_depth.data(), HISTORY_COUNT);
	denoising_resources.vertical_blur_instance->setImageArray("historyMotion", gbuffer_resources.history_motion.data(), HISTORY_COUNT);
	//denoising_resources.vertical_blur_instance->setUniform("historyCamera", &gbuffer_resources.history_camera);

	denoising_resources.vertical_blur_instance->setImage("inputImage", gbuffer_resources.denoiser_temporal_accumulation_texture);
	denoising_resources.vertical_blur_instance->setImage("outputImage", gbuffer_resources.denoiser_blur_texture);

	denoising_resources.horizontal_blur_instance->setImageArray("historyAlbedo", gbuffer_resources.history_albedo.data(), HISTORY_COUNT);
	denoising_resources.horizontal_blur_instance->setImageArray("historyNormalDepth", gbuffer_resources.history_normal_depth.data(), HISTORY_COUNT);
	denoising_resources.horizontal_blur_instance->setImageArray("historyMotion", gbuffer_resources.history_motion.data(), HISTORY_COUNT);
	//denoising_resources.horizontal_blur_instance->setUniform("historyCamera", &gbuffer_resources.history_camera);

	denoising_resources.horizontal_blur_instance->setImage("inputImage", gbuffer_resources.denoiser_blur_texture);
	denoising_resources.horizontal_blur_instance->setImage("outputImage", gbuffer_resources.denoiser_output_texture);
}

void RaytracingScene::createRaytracingResources() {


}

void RaytracingScene::loadAssets() {
	scene_resources.materials = {{
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

	RaytracingModelParserInfo model_parser_info{};
	model_parser_info.mesh_shader_group_index = SHADER_GROUP_TYPE_MESH;
	model_parser_info.acceleration_structures = &scene_resources.mesh_acceleration_structures;
	model_parser_info.materials = &scene_resources.materials;
	model_parser_info.textures = &scene_resources.textures;
	model_parser_info.meshes = &scene_resources.meshes;

	model_parser = new RaytracingModelParser(model_parser_info);

	ModelInfo model_info{};
	model_info.flags = MODEL_FLAG_USED_IN_RAYTRACING;
	model_info.parser = model_parser;
	model_info.path = "/models/sponza/Sponza.gltf";

	sponza_model = Resources::createModel(model_info);

}

void RaytracingScene::createScene() {

	mat4 transform_aabb_floor(1.0f);
	transform_aabb_floor = glm::translate(transform_aabb_floor, vec3(0, -0.5, 0));
	transform_aabb_floor = glm::scale(transform_aabb_floor, vec3(1000, 1, 1000));

	//todo: create a single bottom level acceleration structure from mesh inside the model
	std::vector<AccelerationStructureInstance> sponza_acceleration_structure_instances = sponza_model->getAccelerationStructureInstances();
	scene_resources.acceleration_structure_instances.insert(scene_resources.acceleration_structure_instances.end(), sponza_acceleration_structure_instances.begin(),
	                                                        sponza_acceleration_structure_instances.end());
	scene_resources.acceleration_structure_instances.push_back(AccelerationStructureInstance{0, SHADER_GROUP_TYPE_BOX, transform_aabb_floor, ACCELERATION_STRUCTURE_TYPE_AABB, 0});


	Entity camera_entity = createEntity3D();
	camera_entity.attach<Camera>();
	camera_entity.get<Camera>()->setRenderTarget(Graphics::getDefaultRenderTarget());
	camera_entity.get<Camera>()->active = false;
	camera_entity.get<Transform>()->translate(vec3(0, 1, 0));
	Camera *camera = camera_entity.get<Camera>();
	setCameraEntity(camera_entity);
	gbuffer_resources.history_camera.resize(HISTORY_COUNT, {camera_entity.get<Transform>()->world(), camera->projection});
}

RaytracingScene::RaytracingScene() {
	raytracer = new Raytracer();

	createGBuffer(Graphics::getDefaultRenderTarget()->getResolution().x, Graphics::getDefaultRenderTarget()->getResolution().y);
	createRaytracingResources();
	createDenoisingResources();
	loadAssets();
	createScene();

	AABBAccelerationStructureInfo aabb__acceleration_structure_info{};
	aabb__acceleration_structure_info.max = vec3(0.5, 0.5, 0.5);
	aabb__acceleration_structure_info.min = vec3(-0.5, -0.5, -0.5);
	scene_resources.aabb_acceleration_structure = Resources::createAABBAccelerationStructure(aabb__acceleration_structure_info);

	RootAccelerationStructureInfo root_acceleration_structure_info{};
	root_acceleration_structure_info.aabb_acceleration_structures = &scene_resources.aabb_acceleration_structure;
	root_acceleration_structure_info.aabb_acceleration_structure_count = 1;
	root_acceleration_structure_info.mesh_acceleration_structures = scene_resources.mesh_acceleration_structures.data();
	root_acceleration_structure_info.mesh_acceleration_structure_count = scene_resources.mesh_acceleration_structures.size();
	root_acceleration_structure_info.instances = scene_resources.acceleration_structure_instances.data();
	root_acceleration_structure_info.instance_count = scene_resources.acceleration_structure_instances.size();

	scene_resources.root_acceleration_structure = Resources::createRootAccelerationStructure(root_acceleration_structure_info);

	StorageBufferInfo material_storage_buffer_info{};
	material_storage_buffer_info.stride = sizeof(MaterialData);
	material_storage_buffer_info.count = scene_resources.materials.size();
	material_storage_buffer_info.flags = STORAGE_BUFFER_FLAG_NONE;
	scene_resources.material_buffer = Resources::createStorageBuffer(material_storage_buffer_info);
	scene_resources.material_buffer->update(scene_resources.materials.data());

	std::vector<InstanceInfo> instance_infos;
	for (uint32_t i = 0; i < scene_resources.acceleration_structure_instances.size(); ++i) {
		instance_infos.push_back({scene_resources.acceleration_structure_instances[i].custom_index, i, 16});
	}
	StorageBufferInfo instances_storage_buffer_info{};
	instances_storage_buffer_info.stride = sizeof(InstanceInfo);
	instances_storage_buffer_info.count = instance_infos.size();
	instances_storage_buffer_info.flags = STORAGE_BUFFER_FLAG_NONE;
	scene_resources.instance_buffer = Resources::createStorageBuffer(instances_storage_buffer_info);
	scene_resources.instance_buffer->update(instance_infos.data());


	for (uint32_t i = 0; i < scene_resources.meshes.size(); i++) {
		scene_resources.normals.push_back(scene_resources.meshes[i]->getAttributeStorageBuffer(2));
		scene_resources.indices.push_back(scene_resources.meshes[i]->getIndicesStorageBuffer());
		scene_resources.uvs.push_back(scene_resources.meshes[i]->getAttributeStorageBuffer(1));
	}

	raytracer->setGBufferUniforms(gbuffer_resources);
	raytracer->setSceneUniforms(scene_resources);

	Graphics::getDefaultRenderTarget()->onResolutionChange.subscribe(this, &RaytracingScene::onResolutionChange);
}

void RaytracingScene::createGBuffer(uint32_t width, uint32_t height) {
	for (Image *albedo: gbuffer_resources.history_albedo) {
		delete albedo;
	}
	for (Image *normal_depth: gbuffer_resources.history_normal_depth) {
		delete normal_depth;
	}
	for (Image *motion: gbuffer_resources.history_motion) {
		delete motion;
	}

	for (Image *irradiance: gbuffer_resources.history_irradiance) {
		delete irradiance;
	}
	if (gbuffer_resources.denoiser_blur_texture != nullptr) {
		delete gbuffer_resources.denoiser_blur_texture;
	}
	if (gbuffer_resources.denoiser_temporal_accumulation_texture != nullptr) {
		delete gbuffer_resources.denoiser_temporal_accumulation_texture;
	}
	if (gbuffer_resources.denoiser_output_texture != nullptr) {
		delete gbuffer_resources.denoiser_output_texture;
	}

	gbuffer_resources.history_albedo.clear();
	gbuffer_resources.history_normal_depth.clear();
	gbuffer_resources.history_motion.clear();
	gbuffer_resources.history_irradiance.clear();

	ImageInfo info{};
	info.width = width;
	info.height = height;
	info.format = IMAGE_FORMAT_RGBA32F;
	info.flags = IMAGE_FLAG_SHADER_WRITE;
	info.sampler_info.address_mode = IMAGE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.sampler_info.filter = IMAGE_SAMPLER_FILTER_TYPE_NEAREST;
	info.sampler_info.flags = IMAGE_SAMPLER_FLAG_NONE;

	for (uint32_t i = 0; i < HISTORY_COUNT; i++) {
		gbuffer_resources.history_albedo.push_back(Resources::createTexture(info));
		gbuffer_resources.history_normal_depth.push_back(Resources::createTexture(info));
		gbuffer_resources.history_irradiance.push_back(Resources::createTexture(info));
		gbuffer_resources.history_motion.push_back(Resources::createTexture(info));
	}

	gbuffer_resources.denoiser_temporal_accumulation_texture = Resources::createTexture(info);
	gbuffer_resources.denoiser_blur_texture = Resources::createTexture(info);
	gbuffer_resources.denoiser_output_texture = Resources::createTexture(info);
}

void RaytracingScene::onResolutionChange(RenderTarget *rt) {
	createGBuffer(rt->getResolution().x, rt->getResolution().y);


	Camera *camera = getCameraEntity().get<Camera>();
	camera->setRenderTarget(rt);
}

void RaytracingScene::render() {
	Scene::render();
	Entity camera_entity = getCameraEntity();
	if (!paused)
		frame.time = time;

	mat4 camera_projection = camera_entity.get<Camera>()->projection;
	mat4 camera_view = camera_entity.get<Transform>()->world();
	gbuffer_resources.history_camera[frame.index % HISTORY_COUNT] = {camera_view, camera_projection};

	///--------------------------------RAYTRACING--------------------------------///
	raytracer->tracePrimaryRays(frame, gbuffer_resources, scene_resources.root_acceleration_structure);

	//--------------------------------DENOISING--------------------------------/
	if (render_mode >= ACCUMULATED) {
		//todo:TEMPORAL ACCUMULATION
	}
	if (render_mode >= DENOISED) {
		denoising_resources.vertical_blur_instance->setUniform("frame", &frame, Graphics::getCurrentFrame());
		denoising_resources.horizontal_blur_instance->setUniform("frame", &frame, Graphics::getCurrentFrame());

		ComputeDispatchCmdInfo compute_dispatch_cmd_info{};
		compute_dispatch_cmd_info.pipeline_instance = denoising_resources.vertical_blur_instance;
		compute_dispatch_cmd_info.size_x = gbuffer_resources.denoiser_output_texture->getSize().x;
		compute_dispatch_cmd_info.size_y = gbuffer_resources.denoiser_output_texture->getSize().y;
		compute_dispatch_cmd_info.size_z = 1;
		Graphics::computeDispatch(compute_dispatch_cmd_info);
		compute_dispatch_cmd_info.pipeline_instance = denoising_resources.horizontal_blur_instance;
		Graphics::computeDispatch(compute_dispatch_cmd_info);
	}


	//denoising_resources.vertical_blur_instance->setUniform("camera_history", gbuffer_resources.history_camera.data(), Graphics::getCurrentFrame());
	//denoising_resources.horizontal_blur_instance->setUniform("camera_history", gbuffer_resources.history_camera.data(), Graphics::getCurrentFrame());

	frame.index++;
}

Image *RaytracingScene::getMainCameraTexture() {
	switch (render_mode) {
		case DENOISED:
			return gbuffer_resources.denoiser_output_texture;
			break;
		case ACCUMULATED:
			return gbuffer_resources.denoiser_temporal_accumulation_texture;
			break;
		case IRRADIANCE:
			return gbuffer_resources.history_irradiance[(frame.index - 1) % HISTORY_COUNT];
			break;
		case ALBEDO:
			return gbuffer_resources.history_albedo[(frame.index - 1) % HISTORY_COUNT];
			break;
		case NORMAL:
			return gbuffer_resources.history_normal_depth[(frame.index - 1) % HISTORY_COUNT];
			break;
		default:
			return gbuffer_resources.history_albedo[(frame.index - 1) % HISTORY_COUNT];
	}

}

void RaytracingScene::update(float delta) {
	Scene::update(delta);

	if (Input::getKeyDown(KEY_NUMBER_1)) {
		render_mode = DENOISED;
	}
	if (Input::getKeyDown(KEY_NUMBER_2)) {
		render_mode = ACCUMULATED;
	}
	if (Input::getKeyDown(KEY_NUMBER_3)) {
		render_mode = IRRADIANCE;
	}
	if (Input::getKeyDown(KEY_NUMBER_4)) {
		render_mode = ALBEDO;
	}
	if (Input::getKeyDown(KEY_NUMBER_5)) {
		render_mode = NORMAL;
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
	delete raytracer;

	//GBUFFER
	for (Image *albedo: gbuffer_resources.history_albedo) {
		delete albedo;
	}
	for (Image *normal_depth: gbuffer_resources.history_normal_depth) {
		delete normal_depth;
	}
	for (Image *motion: gbuffer_resources.history_motion) {
		delete motion;
	}

	for (Image *irradiance: gbuffer_resources.history_irradiance) {
		delete irradiance;
	}
	if (gbuffer_resources.denoiser_output_texture != nullptr) {
		delete gbuffer_resources.denoiser_output_texture;
	}
	if (gbuffer_resources.denoiser_blur_texture != nullptr) {
		delete gbuffer_resources.denoiser_blur_texture;
	}
	if (gbuffer_resources.denoiser_temporal_accumulation_texture != nullptr) {
		delete gbuffer_resources.denoiser_temporal_accumulation_texture;
	}

	gbuffer_resources.history_albedo.clear();
	gbuffer_resources.history_normal_depth.clear();
	gbuffer_resources.history_motion.clear();
	gbuffer_resources.history_irradiance.clear();

	//SCENE
	delete scene_resources.root_acceleration_structure;
	delete scene_resources.material_buffer;
	delete scene_resources.instance_buffer;
	delete scene_resources.aabb_acceleration_structure;

	delete sponza_model;
	delete model_parser;

	delete denoising_resources.horizontal_blur_instance;
	delete denoising_resources.vertical_blur_instance;
	delete denoising_resources.horizontal_blur_pipeline;
	delete denoising_resources.vertical_blur_pipeline;
	delete denoising_resources.horizontal_blur_shader;
	delete denoising_resources.vertical_blur_shader;
}

void RaytracingScene::createSphereField(int n) {
	for (uint32_t i = 0; i < n; ++i) {
		float radius = Random::floatRange(0.5, 2.5);
		Transform t{};
		t.translate(vec3(Random::floatRange(-50, 50), radius, Random::floatRange(-50, 50)));
		t.setLocalScale(vec3(radius * 2.0f));
		scene_resources.acceleration_structure_instances.push_back(
				AccelerationStructureInstance{0, SHADER_GROUP_TYPE_SPHERE, t.local(), ACCELERATION_STRUCTURE_TYPE_AABB, Random::uintRange(0, 8)});
	}
}

void RaytracingScene::createMaterialDisplay() {
	for (uint32_t i = 0; i < 10; ++i) {
		mat4 t(1.0f);
		t = glm::translate(t, vec3(2 * (i + 1), 0, 0));
		scene_resources.acceleration_structure_instances.push_back(
				AccelerationStructureInstance{0, SHADER_GROUP_TYPE_BOX, t, ACCELERATION_STRUCTURE_TYPE_AABB, i});
	}
	for (uint32_t i = 0; i < 7; ++i) {
		mat4 t(1.0f);
		t = glm::translate(t, vec3((2 * i) + 1, 0, 0));
		scene_resources.acceleration_structure_instances.push_back(
				AccelerationStructureInstance{0, SHADER_GROUP_TYPE_SPHERE, t, ACCELERATION_STRUCTURE_TYPE_AABB, i % 7});
	}
}





