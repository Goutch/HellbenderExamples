#pragma once

#include "HBE.h"

using namespace HBE;

class TextScene : public Scene {
	std::string text_str = "Hello world!\n"
	                       "This is a test of the text rendering system\n"
	                       "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{};':\",./<>?\\|`~\n"
	                       "You can try to type something now...or zoom in/out\n";

	Shader *text_vertex_shader;
	Shader *text_fragment_shader;
	Mesh *text_mesh;
	RasterizationPipeline *text_pipeline;
	RasterizationPipelineInstance *text_pipeline_instance;
	Font *font;

	void onCharacterInput(char codepoint) {
		text_str += codepoint;
		float total_width;
		float total_height;
		Geometry::updateText(*text_mesh,
		                     text_str,
		                     *font,
		                     1.0,
		                     0.5,
		                     TEXT_ALIGNMENT_CENTER,
		                     PIVOT_CENTER,
		                     total_width,
		                     total_height);

	}

	void update(float delta) {
		Scene::update(delta);

		if (Input::getKeyDown(KEY_ENTER)) {
			onCharacterInput(static_cast<char>('\n'));
		}
		if (Input::getKeyDown(KEY_BACKSPACE)) {
			text_str.pop_back();
			float total_width;
			float total_height;
			Geometry::updateText(*text_mesh,
			                     text_str,
			                     *font,
			                     1.0,
			                     0.5,
			                     TEXT_ALIGNMENT_CENTER,
			                     PIVOT_CENTER,
			                     total_width,
			                     total_height);
		}
		if (Input::getKey(KEY_LEFT)) {
			getCameraEntity().get<Transform>()->translate(vec3(-1 * delta, 0, 0));
		}
		if (Input::getKey(KEY_RIGHT)) {
			getCameraEntity().get<Transform>()->translate(vec3(1 * delta, 0, 0));
		}
		if (Input::getKey(KEY_UP)) {
			getCameraEntity().get<Transform>()->translate(vec3(0, 1 * delta, 0));
		}
		if (Input::getKey(KEY_DOWN)) {
			getCameraEntity().get<Transform>()->translate(vec3(0, -1 * delta, 0));
		}

		getCameraEntity().get<Camera2D>()->setZoomRatio(getCameraEntity().get<Camera2D>()->getZoomRatio() + Input::getMouseWheelInput());
	}

private:
	void createResources() {
		ShaderInfo vertex_shader_info{};
		ShaderInfo fragment_shader_info{};
		vertex_shader_info.stage = SHADER_STAGE_VERTEX;
		fragment_shader_info.stage = SHADER_STAGE_FRAGMENT;

		vertex_shader_info.path = "shaders/defaults/TextMSDF.vert";
		fragment_shader_info.path = "shaders/defaults/TextMSDF.frag";
		text_vertex_shader = Resources::createShader(vertex_shader_info);
		text_fragment_shader = Resources::createShader(fragment_shader_info);

		RasterizationPipelineInfo pipeline_info{};
		pipeline_info.attribute_info_count = 1;

		pipeline_info.attribute_infos = &VERTEX_ATTRIBUTE_INFO_POSITION3D_UV_INTERLEAVED;
		pipeline_info.vertex_shader = text_vertex_shader;
		pipeline_info.fragment_shader = text_fragment_shader;
		pipeline_info.flags = RASTERIZATION_PIPELINE_FLAG_NO_DEPTH_TEST;
		text_pipeline = Resources::createRasterizationPipeline(pipeline_info);

		RasterizationPipelineInstanceInfo pipeline_instance_info{};
		pipeline_instance_info.flags = GRAPHIC_PIPELINE_INSTANCE_FLAG_NONE;

		pipeline_instance_info.rasterization_pipeline = text_pipeline;
		text_pipeline_instance = Resources::createRasterizationPipelineInstance(pipeline_instance_info);

		std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{};':\",./<>?\\|`~";
		FontInfo font_info{};
		font_info.path = "fonts/Roboto-Regular.ttf";
		font_info.characters = characters.data();
		font_info.characters_count = characters.size();
		font_info.glyph_resolution = 64;

		font = Resources::createFont(font_info);

		text_pipeline_instance->setImage("mtsdf", font->getTextureAtlas());
		float total_width;
		float total_height;
		text_mesh = Geometry::createText(text_str,
		                                 *font,
		                                 1.0,
		                                 0.5,
		                                 TEXT_ALIGNMENT_CENTER,
		                                 PIVOT_CENTER,
		                                 total_width,
		                                 total_height);

		vec4 color = vec4(1, 1, 1, 1);
		text_pipeline_instance->setUniform("material", &color);
	}

	void setupScene() {
		Entity camera_entity = createEntity3D();

		Camera2D *camera2D = camera_entity.attach<Camera2D>();
		camera2D->setZoomRatio(35.0f);

		Entity text_entity = createEntity3D();
		MeshRenderer *text_renderer = text_entity.attach<MeshRenderer>();
		text_renderer->ordered = true;
		text_renderer->layer = 0;
		text_renderer->pipeline_instance = text_pipeline_instance;
		text_renderer->mesh = text_mesh;
	}

public:
	TextScene() {
		createResources();
		setupScene();
		Input::onCharDown.subscribe(this, &TextScene::onCharacterInput);
	}

	~TextScene() {
		delete text_vertex_shader;
		delete text_fragment_shader;
		delete text_pipeline_instance;
		delete text_pipeline;
		delete font;
		delete text_mesh;
	}

};