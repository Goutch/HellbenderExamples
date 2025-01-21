
#include "Denoiser.h"

void Denoiser::accumulate(Frame &frame, GBufferResources &gbuffer_resources) {
	denoiser_resources.temporal_accumulation_instance->setUniform("frame", &frame, Graphics::getCurrentFrame());
	denoiser_resources.temporal_accumulation_instance->setUniform("camera_history", gbuffer_resources.history_camera.data(), Graphics::getCurrentFrame());
	ComputeDispatchCmdInfo compute_dispatch_cmd_info{};
	compute_dispatch_cmd_info.pipeline_instance = denoiser_resources.temporal_accumulation_instance;
	compute_dispatch_cmd_info.size_x = gbuffer_resources.history_albedo[0]->getSize().x;
	compute_dispatch_cmd_info.size_y = gbuffer_resources.history_albedo[0]->getSize().y;
	Graphics::computeDispatch(compute_dispatch_cmd_info);
}

void Denoiser::setSceneUniforms(SceneResources &scene_resources) {
	denoiser_resources.temporal_accumulation_instance->setUniform("camera_history", scene_resources..data(), Graphics::getCurrentFrame());
}

void Denoiser::setGBufferUniforms(GBufferResources &gbuffer_resources) {
	denoiser_resources.temporal_accumulation_instance->setImageArray("historyAlbedo", gbuffer_resources.history_albedo.data(), gbuffer_resources.history_albedo.size());
	denoiser_resources.temporal_accumulation_instance->setImageArray("historyNormalDepth", gbuffer_resources.history_normal_depth.data(), gbuffer_resources.history_normal_depth.size());
	denoiser_resources.temporal_accumulation_instance->setImageArray("historyMotion", gbuffer_resources.history_motion.data(), gbuffer_resources.history_motion.size());
	denoiser_resources.temporal_accumulation_instance->setImageArray("historyIrradiance", gbuffer_resources.history_irradiance.data(), gbuffer_resources.history_irradiance.size());
}

Denoiser::Denoiser() {
	ShaderInfo shader_info{};
	shader_info.stage = SHADER_STAGE_COMPUTE;
	shader_info.path = "shaders/denoiser/temporal_accumulation.comp";
	denoiser_resources.temporal_accumulation_shader = Resources::createShader(shader_info);

	ComputePipelineInfo compute_pipeline_info{};
	compute_pipeline_info.compute_shader = denoiser_resources.temporal_accumulation_shader;
	denoiser_resources.temporal_accumulation_pipeline = Resources::createComputePipeline(compute_pipeline_info);

	ComputeInstanceInfo compute_pipeline_instance_info{};
	compute_pipeline_instance_info.compute_pipeline = denoiser_resources.temporal_accumulation_pipeline;
	denoiser_resources.temporal_accumulation_instance = Resources::createComputeInstance(compute_pipeline_instance_info);
}

Denoiser::~Denoiser() {
	delete denoiser_resources.temporal_accumulation_instance;
	delete denoiser_resources.temporal_accumulation_pipeline;
	delete denoiser_resources.temporal_accumulation_shader;
}
