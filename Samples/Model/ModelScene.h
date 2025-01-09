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
		attribute_infos[1].size = sizeof(vec2);
		attribute_infos[1].flags = VERTEX_ATTRIBUTE_FLAG_NONE;

		attribute_infos.emplace_back();
		attribute_infos[2].location = 2;
		attribute_infos[2].size = sizeof(vec3);
		attribute_infos[2].flags = VERTEX_ATTRIBUTE_FLAG_NONE;

		RasterizationPipelineInfo pipeline_info{};
		pipeline_info.attribute_infos = attribute_infos.data();
		pipeline_info.attribute_info_count = attribute_infos.size();
		pipeline_info.fragment_shader = frag;
		pipeline_info.vertex_shader = vert;

		pipeline_info.flags = RASTERIZATION_PIPELINE_FLAG_FRONT_COUNTER_CLOCKWISE | //gltf primitives are counterclockwise
			RASTERIZATION_PIPELINE_FLAG_ALLOW_EMPTY_DESCRIPTOR;
		RasterizationPipeline* model_pipeline_2_sided = Resources::createRasterizationPipeline(
				pipeline_info, "MODEL_PIPELINE_2_SIDED");
		pipeline_info.flags |= RASTERIZATION_PIPELINE_FLAG_CULL_BACK;
		RasterizationPipeline* model_pipeline = Resources::createRasterizationPipeline(pipeline_info, "MODEL_PIPELINE");

		DefaultModelParserInfo parser_info{};
		parser_info.graphic_pipeline = model_pipeline;
		parser_info.graphic_pipeline_2_sided = model_pipeline_2_sided;
		parser_info.texture_names.emplace(MODEL_TEXTURE_TYPE_ALBEDO, "albedo");
		parser_info.material_property_name = "material";
		DefaultModelParser parser = DefaultModelParser(parser_info);

		ModelInfo model_info{};
		model_info.flags = MODEL_FLAG_NONE;
		model_info.parser = &parser;

		Log::debug("Loading sponza model");
		model_info.path = "/models/sponza/Sponza.gltf";
		Model* sponza_model = Resources::createModel(model_info, "sponza");

		Log::debug("Loading beautiful-game model");
		model_info.path = "/models/beautiful-game/ABeautifulGame.gltf";
		Model* chess_model = Resources::createModel(model_info, "chess");


		//-------------------SCENE CREATION--------------------------------------

		//Create camera
		Entity camera_entity = createEntity3D();
		Camera* camera = camera_entity.attach<Camera>();
		CameraController* camera_controller = camera_entity.attach<CameraController>();
		camera_controller->speed = 10;
		camera_entity.get<Transform>()->translate(vec3(0, 2, 0));
		camera->setRenderTarget(Graphics::getDefaultRenderTarget());
		setCameraEntity(camera_entity);

		auto chess = createEntity3D();
		ModelRenderer* chess_renderer = chess.attach<ModelRenderer>();
		chess_renderer->model = chess_model;
		chess.get<Transform>()->setLocalScale(vec3(3));
		chess.get<Transform>()->translate(vec3(0, 0, 0));

		auto sponza = createEntity3D();
		ModelRenderer* sponza_renderer = sponza.attach<ModelRenderer>();
		sponza_renderer->model = sponza_model;
		sponza.get<Transform>()->setLocalScale(vec3(1));
		sponza.get<Transform>()->translate(vec3(0, 0, 0));
	}
};
