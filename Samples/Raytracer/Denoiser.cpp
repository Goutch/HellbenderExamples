
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

void Denoiser::blurIrradiance(Frame &frame, GBufferResources &gbuffer_resources) {

	denoiser_resources.vertical_blur_instance->setUniform("frame", &frame, Graphics::getCurrentFrame());
	denoiser_resources.horizontal_blur_instance->setUniform("frame", &frame, Graphics::getCurrentFrame());

	denoiser_resources.vertical_blur_instance->setImage("inputImage", gbuffer_resources.history_irradiance[frame.index % gbuffer_resources.history_irradiance.size()], Graphics::getCurrentFrame());
	denoiser_resources.vertical_blur_instance->setImage("outputImage", gbuffer_resources.denoiser_irradiance_vertical_blur_texture, Graphics::getCurrentFrame());

	denoiser_resources.horizontal_blur_instance->setImage("inputImage", gbuffer_resources.denoiser_irradiance_vertical_blur_texture, Graphics::getCurrentFrame());
	denoiser_resources.horizontal_blur_instance->setImage("outputImage", gbuffer_resources.history_irradiance[frame.index % gbuffer_resources.history_irradiance.size()], Graphics::getCurrentFrame());

	ComputeDispatchCmdInfo compute_dispatch_cmd_info{};
	compute_dispatch_cmd_info.pipeline_instance = denoiser_resources.vertical_blur_instance;
	compute_dispatch_cmd_info.size_x = gbuffer_resources.denoiser_irradiance_vertical_blur_texture->getSize().x;
	compute_dispatch_cmd_info.size_y = gbuffer_resources.denoiser_irradiance_vertical_blur_texture->getSize().y;
	compute_dispatch_cmd_info.size_z = 1;
	Graphics::computeDispatch(compute_dispatch_cmd_info);
	compute_dispatch_cmd_info.pipeline_instance = denoiser_resources.horizontal_blur_instance;
	Graphics::computeDispatch(compute_dispatch_cmd_info);
}

void Denoiser::setSceneUniforms(SceneResources &scene_resources) {
}

void Denoiser::setGBufferUniforms(GBufferResources &gbuffer_resources) {
	denoiser_resources.temporal_accumulation_instance->setImageArray("historyAlbedo", gbuffer_resources.history_albedo.data(), gbuffer_resources.history_albedo.size());
	denoiser_resources.temporal_accumulation_instance->setImageArray("historyNormalDepth", gbuffer_resources.history_normal_depth.data(), gbuffer_resources.history_normal_depth.size());
	denoiser_resources.temporal_accumulation_instance->setImageArray("historyMotion", gbuffer_resources.history_motion.data(), gbuffer_resources.history_motion.size());
	denoiser_resources.temporal_accumulation_instance->setImageArray("historyIrradiance", gbuffer_resources.history_irradiance.data(), gbuffer_resources.history_irradiance.size());
	denoiser_resources.temporal_accumulation_instance->setImageArray("historyPosition", gbuffer_resources.history_position.data(), gbuffer_resources.history_position.size(), -1, 0);

	denoiser_resources.temporal_accumulation_instance->setImage("outputImage", gbuffer_resources.denoiser_temporal_accumulation_texture);

	denoiser_resources.vertical_blur_instance->setImageArray("historyAlbedo", gbuffer_resources.history_albedo.data(), gbuffer_resources.history_albedo.size());
	denoiser_resources.vertical_blur_instance->setImageArray("historyNormalDepth", gbuffer_resources.history_normal_depth.data(), gbuffer_resources.history_normal_depth.size());
	denoiser_resources.vertical_blur_instance->setImageArray("historyMotion", gbuffer_resources.history_motion.data(), gbuffer_resources.history_motion.size());
	denoiser_resources.vertical_blur_instance->setImageArray("historyPosition", gbuffer_resources.history_position.data(), gbuffer_resources.history_position.size());


	denoiser_resources.horizontal_blur_instance->setImageArray("historyAlbedo", gbuffer_resources.history_albedo.data(), gbuffer_resources.history_albedo.size());
	denoiser_resources.horizontal_blur_instance->setImageArray("historyNormalDepth", gbuffer_resources.history_normal_depth.data(), gbuffer_resources.history_normal_depth.size());
	denoiser_resources.horizontal_blur_instance->setImageArray("historyMotion", gbuffer_resources.history_motion.data(), gbuffer_resources.history_motion.size());
	denoiser_resources.horizontal_blur_instance->setImageArray("historyPosition", gbuffer_resources.history_position.data(), gbuffer_resources.history_position.size());
}

Denoiser::Denoiser() {
	//Accumulation
	ShaderInfo shader_info{};
	shader_info.stage = SHADER_STAGE_COMPUTE;
	shader_info.path = "shaders/denoising/temporal_accumulation.comp";
	denoiser_resources.temporal_accumulation_shader = Resources::createShader(shader_info);

	ComputePipelineInfo accumulation_compute_pipeline_info{};
	accumulation_compute_pipeline_info.compute_shader = denoiser_resources.temporal_accumulation_shader;
	denoiser_resources.temporal_accumulation_pipeline = Resources::createComputePipeline(accumulation_compute_pipeline_info);

	ComputeInstanceInfo accumulation_compute_pipeline_instance_info{};
	accumulation_compute_pipeline_instance_info.compute_pipeline = denoiser_resources.temporal_accumulation_pipeline;
	denoiser_resources.temporal_accumulation_instance = Resources::createComputeInstance(accumulation_compute_pipeline_instance_info);


	//BLUR
	shader_info.path = "shaders/denoising/vertical.comp";

	shader_info.stage = SHADER_STAGE_COMPUTE;
	shader_info.path = "shaders/denoising/horizontal.comp";
	denoiser_resources.horizontal_blur_shader = Resources::createShader(shader_info);
	shader_info.path = "shaders/denoising/vertical.comp";
	denoiser_resources.vertical_blur_shader = Resources::createShader(shader_info);

	ComputePipelineInfo blur_compute_pipeline_info{};
	blur_compute_pipeline_info.compute_shader = denoiser_resources.horizontal_blur_shader;
	denoiser_resources.horizontal_blur_pipeline = Resources::createComputePipeline(blur_compute_pipeline_info);
	blur_compute_pipeline_info.compute_shader = denoiser_resources.vertical_blur_shader;
	denoiser_resources.vertical_blur_pipeline = Resources::createComputePipeline(blur_compute_pipeline_info);

	ComputeInstanceInfo blur_compute_instance_info{};
	blur_compute_instance_info.compute_pipeline = denoiser_resources.horizontal_blur_pipeline;
	denoiser_resources.horizontal_blur_instance = Resources::createComputeInstance(blur_compute_instance_info);

	blur_compute_instance_info.compute_pipeline = denoiser_resources.vertical_blur_pipeline;
	denoiser_resources.vertical_blur_instance = Resources::createComputeInstance(blur_compute_instance_info);
}

Denoiser::~Denoiser() {
	delete denoiser_resources.temporal_accumulation_instance;
	delete denoiser_resources.temporal_accumulation_pipeline;
	delete denoiser_resources.temporal_accumulation_shader;

	delete denoiser_resources.horizontal_blur_instance;
	delete denoiser_resources.horizontal_blur_pipeline;
	delete denoiser_resources.horizontal_blur_shader;

	delete denoiser_resources.vertical_blur_instance;
	delete denoiser_resources.vertical_blur_pipeline;
	delete denoiser_resources.vertical_blur_shader;

}
