
#pragma once
#include "DataStructures.h"

class Denoiser {

private :
	DenoisingResources denoiser_resources;
public:
	Denoiser();
	~Denoiser();
	void setGBufferUniforms(GBufferResources &gbuffer_resources);

	void setSceneUniforms(SceneResources &scene_resources);

	void accumulate(Frame &frame, GBufferResources &gbuffer_resources);
};
