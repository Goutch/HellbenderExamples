
#pragma once

#include "DataStructures.h"

class RaytracingScene;

class Raytracer {
	RaytracingScene *scene;
	RaytracerResources primary_raytracing_resources;
public:
	Raytracer();

	~Raytracer();

	void traceRays(Frame &frame, GBufferResources &gbuffer_resources, RootAccelerationStructure *root_acceleration_structure);

	void setGBufferUniforms(GBufferResources &gbuffer_resources);

	void setSceneUniforms(SceneResources &scene_resources);

	RaytracerResources &getRaytracingResources();

private:
	void createPrimaryRaytracingResources();
};
