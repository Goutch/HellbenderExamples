#include "Raytracer.h"
#include "RaytracingScene.h"

Raytracer::Raytracer() {
	createPrimaryRaytracingResources();

	raytracing_resources.pipeline_instance->setImageArray("blue_noise", raytracing_resources.st_blue_noise.data(), raytracing_resources.st_blue_noise.size());
}


Raytracer::~Raytracer() {
	delete raytracing_resources.pipeline_instance;
	delete raytracing_resources.pipeline;
	delete raytracing_resources.raygen_shader;
	for (Shader *shader: raytracing_resources.miss_shaders) {
		delete shader;
	}
	for (Shader *shader: raytracing_resources.hit_shaders) {
		delete shader;
	}
	for (Image *image: raytracing_resources.st_blue_noise) {
		delete image;
	}
}

void Raytracer::traceRays(Frame &frame, GBufferResources &gbuffer_resources, RootAccelerationStructure *root_acceleration_structure) {
	raytracing_resources.pipeline_instance->setUniform("frame", &frame, Graphics::getCurrentFrame());
	raytracing_resources.pipeline_instance->setUniform("camera_history", gbuffer_resources.history_camera.data(), Graphics::getCurrentFrame());

	TraceRaysCmdInfo trace_rays_cmd_info{};
	trace_rays_cmd_info.pipeline_instance = raytracing_resources.pipeline_instance;
	trace_rays_cmd_info.root_acceleration_structure = root_acceleration_structure;
	trace_rays_cmd_info.resolution = gbuffer_resources.history_albedo[0]->getSize();
	Graphics::traceRays(trace_rays_cmd_info);
}

void Raytracer::setGBufferUniforms(GBufferResources &gbuffer_resources) {
	raytracing_resources.pipeline_instance->setImageArray("historyAlbedo", gbuffer_resources.history_albedo.data(), gbuffer_resources.history_albedo.size(), -1, 0);
	raytracing_resources.pipeline_instance->setImageArray("historyNormalDepth", gbuffer_resources.history_normal_depth.data(),
	                                                      gbuffer_resources.history_normal_depth.size(), -1, 0);
	raytracing_resources.pipeline_instance->setImageArray("historyMotion", gbuffer_resources.history_motion.data(), gbuffer_resources.history_motion.size(), -1, 0);
	raytracing_resources.pipeline_instance->setImageArray("historyIrradiance", gbuffer_resources.history_irradiance.data(), gbuffer_resources.history_irradiance.size(), -1,
	                                                      0);
	raytracing_resources.pipeline_instance->setImageArray("historyPosition", gbuffer_resources.history_position.data(), gbuffer_resources.history_position.size(), -1, 0);
}

RaytracerResources &Raytracer::getRaytracingResources() {
	return raytracing_resources;
}

void Raytracer::setSceneUniforms(SceneResources &scene_resources) {
	if (scene_resources.textures.size() > 0)
		raytracing_resources.pipeline_instance->setImageArray("textures", scene_resources.textures.data(), scene_resources.textures.size());
	raytracing_resources.pipeline_instance->setAccelerationStructure("topLevelAS", scene_resources.root_acceleration_structure);
	raytracing_resources.pipeline_instance->setStorageBuffer("instances", scene_resources.instance_buffer, scene_resources.instance_buffer->getCount(), 0);
	raytracing_resources.pipeline_instance->setStorageBufferArray("mesh_indices_buffers", scene_resources.indices.data(), scene_resources.indices.size(), -1);
	raytracing_resources.pipeline_instance->setStorageBufferArray("mesh_normals_buffers", scene_resources.normals.data(), scene_resources.normals.size(), -1);
	raytracing_resources.pipeline_instance->setStorageBufferArray("mesh_tex_coords_buffers", scene_resources.uvs.data(), scene_resources.uvs.size(), -1);
	raytracing_resources.pipeline_instance->setStorageBuffer("materials", scene_resources.material_buffer, scene_resources.material_buffer->getCount(), 0);
}


void Raytracer::createPrimaryRaytracingResources() {
	ShaderInfo shader_info{};
	shader_info.stage = SHADER_STAGE_RAY_GEN;
	shader_info.path = "shaders/raytracing/raygen/raygen.glsl";
	raytracing_resources.raygen_shader = Resources::createShader(shader_info);

	shader_info.stage = SHADER_STAGE_RAY_MISS;
	shader_info.path = "shaders/raytracing/miss/primary_miss.glsl";
	raytracing_resources.miss_shaders.push_back(Resources::createShader(shader_info));

	shader_info.stage = SHADER_STAGE_CLOSEST_HIT;
	shader_info.path = "shaders/raytracing/closestHit/primary_closest_hit_aabb.glsl";
	raytracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));

	shader_info.path = "shaders/raytracing/closestHit/primary_closest_hit_mesh.glsl";
	raytracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));

	shader_info.stage = SHADER_STAGE_INTERSECTION;
	shader_info.path = "shaders/raytracing/intersect/intersect_box.glsl";
	raytracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));
	shader_info.path = "shaders/raytracing/intersect/intersect_sphere.glsl";
	raytracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));

	//primary rays
	raytracing_resources.shader_groups.push_back({0, -1, 2}); //box
	raytracing_resources.shader_groups.push_back({0, -1, 3}); //sphere
	raytracing_resources.shader_groups.push_back({1, -1, -1}); //Mesh

	RaytracingPipelineInfo primary_raytraycing_pipeline_info{};

	primary_raytraycing_pipeline_info.raygen_shader = raytracing_resources.raygen_shader;
	primary_raytraycing_pipeline_info.miss_shaders = raytracing_resources.miss_shaders.data();
	primary_raytraycing_pipeline_info.miss_shader_count = raytracing_resources.miss_shaders.size();
	primary_raytraycing_pipeline_info.hit_shaders = raytracing_resources.hit_shaders.data();
	primary_raytraycing_pipeline_info.hit_shader_count = raytracing_resources.hit_shaders.size();

	primary_raytraycing_pipeline_info.shader_group_count = raytracing_resources.shader_groups.size();
	primary_raytraycing_pipeline_info.shader_groups = raytracing_resources.shader_groups.data();
	primary_raytraycing_pipeline_info.max_recursion_depth = 1;

	raytracing_resources.pipeline = Resources::createRaytracingPipeline(primary_raytraycing_pipeline_info);

	RaytracingPipelineInstanceInfo raytracing_pipeline_instance_info{};
	raytracing_pipeline_instance_info.raytracing_pipeline = raytracing_resources.pipeline;
	raytracing_resources.pipeline_instance = Resources::createRaytracingPipelineInstance(raytracing_pipeline_instance_info);

	ImageInfo blue_noise_info{};
	blue_noise_info.flags = IMAGE_FLAG_NO_SAMPLER;
	blue_noise_info.format = IMAGE_FORMAT_RGBA8_UNORM;
	blue_noise_info.generate_mip_maps = false;

	std::string blue_noise_path = "textures/stbn_unitvec3_cosine_2Dx1D_128x128x64_";
	for (uint32_t i = 0; i < 64; i++) {
		raytracing_resources.st_blue_noise.push_back(Image::load(blue_noise_path + std::to_string(i) + ".png", blue_noise_info));
	}
}
