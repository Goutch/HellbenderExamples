#pragma once

#include "HBE.h"

using namespace HBE;


class ModelScene : public Scene
{
public:
	ModelScene()
	{
		//-------------------RESOURCES CREATION--------------------------------------
		ShaderInfo frag_info{SHADER_STAGE_FRAGMENT, "/shaders/defaults/PositionUVNormalTextured.frag"};
		ShaderInfo vert_info{SHADER_STAGE_VERTEX, "/shaders/defaults/PositionUVNormal.vert"};
		auto frag = Resources::createShader(frag_info, "frag");
		auto vert = Resources::createShader(vert_info, "vert");

		std::vector<VertexAttributeInfo> attribute_infos;
		//vertex binding
		attribute_infos.emplace_back();
		attribute_infos[0].location = 0;
		attribute_infos[0].size = sizeof(vec3);
		attribute_infos[0].flags = VERTEX_ATTRIBUTE_FLAG_NONE;

		attribute_infos.emplace_back();
		attribute_infos[1].location = 1;
		attribute_infos[1].size = sizeof(vec3);
		attribute_infos[1].flags = VERTEX_ATTRIBUTE_FLAG_NONE;

		GraphicPipelineInfo pipeline_info{};
		pipeline_info.attribute_infos = attribute_infos.data();
		pipeline_info.attribute_info_count = attribute_infos.size();
		pipeline_info.fragment_shader = frag;
		pipeline_info.vertex_shader = vert;

		pipeline_info.flags = GRAPHIC_PIPELINE_FLAG_FRONT_COUNTER_CLOCKWISE | //gltf primitives are counterclockwise
			GRAPHIC_PIPELINE_FLAG_ALLOW_EMPTY_DESCRIPTOR;
		GraphicPipeline* model_pipeline_2_sided = Resources::createGraphicPipeline(
			pipeline_info, "MODEL_PIPELINE_2_SIDED");
		pipeline_info.flags |= GRAPHIC_PIPELINE_FLAG_CULL_BACK;
		GraphicPipeline* model_pipeline = Resources::createGraphicPipeline(pipeline_info, "MODEL_PIPELINE");

		DefaultModelParserInfo parser_info{};
		parser_info.graphic_pipeline = model_pipeline;
		parser_info.graphic_pipeline_2_sided = model_pipeline_2_sided;
		parser_info.texture_names.emplace(MODEL_TEXTURE_TYPE_ALBEDO, "albedo");
		parser_info.material_property_name = "material";
		DefaultModelParser parser = DefaultModelParser(parser_info);

		ModelInfo model_info{};
		model_info.flags = MODEL_FLAG_NONE;
		model_info.parser = &parser;

		Log::debug("Loading cube model");
		model_info.path = "/models/cube/Cube.gltf";
		Model *cube_model = Resources::createModel(model_info, "cube");

		Log::debug("Loading sponza model");
		model_info.path = "/models/sponza/Sponza.gltf";
		//Model *sponza_model = Resources::createModel(model_info, "sponza");

		Log::debug("Loading teapot model");
		model_info.path = "/models/teapot.gltf";
		//Model* teapot_model = Resources::createModel(model_info, "teapot");

		Log::debug("Loading dragon model");
		model_info.path = "/models/dragon.gltf";
		//Model* dragon_model = Resources::createModel(model_info, "dragon");

		Log::debug("Loading avocado model");
		model_info.path = "/models/avocado/Avocado.gltf";
		//Model* avocado_model = Resources::createModel(model_info, "avocado");

		//-------------------SCENE CREATION--------------------------------------

		//Create camera
		Entity camera_entity = createEntity3D();
		Camera* camera = camera_entity.attach<Camera>();
		CameraController* camera_controller = camera_entity.attach<CameraController>();
		camera_controller->invert_y = true;
		camera_controller->speed = 10;
		camera->setRenderTarget(Graphics::getDefaultRenderTarget());
		setCameraEntity(camera_entity);

		//auto teapot = createEntity3D();
		//ModelRenderer *teapot_renderer = teapot.attach<ModelRenderer>();
		//teapot.get<Transform>()->translate(vec3(0, 0, -5));
		//teapot.get<Transform>()->setLocalScale(vec3(0.1));
		//teapot_renderer->model = teapot_model;

		//auto sponza = createEntity3D();
		//ModelRenderer *sponza_renderer = sponza.attach<ModelRenderer>();
		//sponza_renderer->model = sponza_model;
		//sponza.get<Transform>()->setLocalScale(vec3(0.01));

		auto cube = createEntity3D();
		ModelRenderer* cube_renderer = cube.attach<ModelRenderer>();
		cube.get<Transform>()->translate(vec3(0, 0, -2));
		cube.get<Transform>()->setLocalScale(vec3(0.5));
		cube_renderer->model = cube_model;

		//auto donut = createEntity3D();
		//ModelRenderer *donut_renderer = donut.attach<ModelRenderer>();
		//donut.get<Transform>()->translate(vec3(0, 0, -5));
		//donut.get<Transform>()->setLocalScale(vec3(0.1));
		//donut_renderer->model = donut_model;


		//auto avocado = createEntity3D();
		//ModelRenderer* avocado_renderer = avocado.attach<ModelRenderer>();
		//avocado.get<Transform>()->translate(vec3(0, 0, -2));
		//avocado.get<Transform>()->setLocalScale(vec3(50));
		//avocado_renderer->model = avocado_model;

		//auto dragon = createEntity3D();
		//ModelRenderer *dragon_renderer = dragon.attach<ModelRenderer>();
		//dragon.get<Transform>()->translate(vec3(0, 0, -10));
		//dragon_renderer->model = dragon_model;
	}
};
