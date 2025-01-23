
#include "Raytracer.h"
#include "RaytracingScene.h"

Raytracer::Raytracer() {
	createPrimaryRaytracingResources();
}


Raytracer::~Raytracer() {
	delete primary_raytracing_resources.pipeline_instance;
	delete primary_raytracing_resources.pipeline;
	delete primary_raytracing_resources.raygen_shader;
	for (Shader *shader: primary_raytracing_resources.miss_shaders) {
		delete shader;
	}
	for (Shader *shader: primary_raytracing_resources.hit_shaders) {
		delete shader;
	}
}

void Raytracer::traceRays(Frame &frame, GBufferResources &gbuffer_resources, RootAccelerationStructure *root_acceleration_structure) {
	primary_raytracing_resources.pipeline_instance->setUniform("frame", &frame, Graphics::getCurrentFrame());
	primary_raytracing_resources.pipeline_instance->setUniform("camera_history", gbuffer_resources.history_camera.data(), Graphics::getCurrentFrame());

	TraceRaysCmdInfo trace_rays_cmd_info{};
	trace_rays_cmd_info.pipeline_instance = primary_raytracing_resources.pipeline_instance;
	trace_rays_cmd_info.root_acceleration_structure = root_acceleration_structure;
	trace_rays_cmd_info.resolution = gbuffer_resources.history_albedo[0]->getSize();
	Graphics::traceRays(trace_rays_cmd_info);
}

void Raytracer::setGBufferUniforms(GBufferResources &gbuffer_resources) {
	primary_raytracing_resources.pipeline_instance->setImageArray("historyAlbedo", gbuffer_resources.history_albedo.data(), gbuffer_resources.history_albedo.size(), -1, 0);
	primary_raytracing_resources.pipeline_instance->setImageArray("historyNormalDepth", gbuffer_resources.history_normal_depth.data(), gbuffer_resources.history_normal_depth.size(), -1, 0);
	primary_raytracing_resources.pipeline_instance->setImageArray("historyMotion", gbuffer_resources.history_motion.data(), gbuffer_resources.history_motion.size(), -1, 0);
	primary_raytracing_resources.pipeline_instance->setImageArray("historyIrradiance", gbuffer_resources.history_irradiance.data(), gbuffer_resources.history_irradiance.size(), -1, 0);
	primary_raytracing_resources.pipeline_instance->setImageArray("historyPosition", gbuffer_resources.history_position.data(), gbuffer_resources.history_position.size(), -1, 0);
}

RaytracerResources &Raytracer::getRaytracingResources() {
	return primary_raytracing_resources;
}

void Raytracer::setSceneUniforms(SceneResources &scene_resources) {
	if (scene_resources.textures.size() > 0)
		primary_raytracing_resources.pipeline_instance->setImageArray("textures", scene_resources.textures.data(), scene_resources.textures.size());
	primary_raytracing_resources.pipeline_instance->setAccelerationStructure("topLevelAS", scene_resources.root_acceleration_structure);
	primary_raytracing_resources.pipeline_instance->setStorageBuffer("instances", scene_resources.instance_buffer, scene_resources.instance_buffer->getCount(), 0);
	primary_raytracing_resources.pipeline_instance->setStorageBufferArray("mesh_indices_buffers", scene_resources.indices.data(), scene_resources.indices.size(), -1);
	primary_raytracing_resources.pipeline_instance->setStorageBufferArray("mesh_normals_buffers", scene_resources.normals.data(), scene_resources.normals.size(), -1);
	primary_raytracing_resources.pipeline_instance->setStorageBufferArray("mesh_tex_coords_buffers", scene_resources.uvs.data(), scene_resources.uvs.size(), -1);
	primary_raytracing_resources.pipeline_instance->setStorageBuffer("materials", scene_resources.material_buffer, scene_resources.material_buffer->getCount(), 0);
}


void Raytracer::createPrimaryRaytracingResources() {
	ShaderInfo shader_info{};
	shader_info.stage = SHADER_STAGE_RAY_GEN;
	shader_info.path = "shaders/raytracing/raygen/raygen.glsl";
	primary_raytracing_resources.raygen_shader = Resources::createShader(shader_info);

	shader_info.stage = SHADER_STAGE_RAY_MISS;
	shader_info.path = "shaders/raytracing/miss/primary_miss.glsl";
	primary_raytracing_resources.miss_shaders.push_back(Resources::createShader(shader_info));

	shader_info.stage = SHADER_STAGE_CLOSEST_HIT;
	shader_info.path = "shaders/raytracing/closestHit/primary_closest_hit_aabb.glsl";
	primary_raytracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));

	shader_info.path = "shaders/raytracing/closestHit/primary_closest_hit_mesh.glsl";
	primary_raytracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));

	shader_info.stage = SHADER_STAGE_INTERSECTION;
	shader_info.path = "shaders/raytracing/intersect/intersect_box.glsl";
	primary_raytracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));
	shader_info.path = "shaders/raytracing/intersect/intersect_sphere.glsl";
	primary_raytracing_resources.hit_shaders.push_back(Resources::createShader(shader_info));

	//primary rays
	primary_raytracing_resources.shader_groups.push_back({0, -1, 2});//box
	primary_raytracing_resources.shader_groups.push_back({0, -1, 3});//sphere
	primary_raytracing_resources.shader_groups.push_back({1, -1, -1});//Mesh

	RaytracingPipelineInfo primary_raytraycing_pipeline_info{};

	primary_raytraycing_pipeline_info.raygen_shader = primary_raytracing_resources.raygen_shader;
	primary_raytraycing_pipeline_info.miss_shaders = primary_raytracing_resources.miss_shaders.data();
	primary_raytraycing_pipeline_info.miss_shader_count = primary_raytracing_resources.miss_shaders.size();
	primary_raytraycing_pipeline_info.hit_shaders = primary_raytracing_resources.hit_shaders.data();
	primary_raytraycing_pipeline_info.hit_shader_count = primary_raytracing_resources.hit_shaders.size();

	primary_raytraycing_pipeline_info.shader_group_count = primary_raytracing_resources.shader_groups.size();
	primary_raytraycing_pipeline_info.shader_groups = primary_raytracing_resources.shader_groups.data();
	primary_raytraycing_pipeline_info.max_recursion_depth = 4;

	primary_raytracing_resources.pipeline = Resources::createRaytracingPipeline(primary_raytraycing_pipeline_info);

	RaytracingPipelineInstanceInfo raytracing_pipeline_instance_info{};
	raytracing_pipeline_instance_info.raytracing_pipeline = primary_raytracing_resources.pipeline;
	primary_raytracing_resources.pipeline_instance = Resources::createRaytracingPipelineInstance(raytracing_pipeline_instance_info);
}


