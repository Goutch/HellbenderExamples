#include "HBE.h"

using namespace HBE;

class InstancingScene : public Scene {
	int range = 100;
	const int num_instances = 1000000;//1 million
	mat4 *instance_transforms;
	Mesh *cube_mesh;
	Shader *fragment_shader;
	Shader *vertex_shader;
	RasterizationPipeline *pipeline;
	RasterizationPipelineInstance *pipeline_instance;
public:

	InstancingScene() {
		createResources();
		setupScene();
	}

	~InstancingScene() {
		delete cube_mesh;
		delete fragment_shader;
		delete vertex_shader;
		delete pipeline;
		delete pipeline_instance;
		delete[] instance_transforms;
	}

	void createResources() {
		ShaderInfo shader_info{};
		shader_info.path = "shaders/InstancedPosition.frag";
		shader_info.stage = SHADER_STAGE_FRAGMENT;
		fragment_shader = Resources::createShader(shader_info);

		shader_info.path = "shaders/InstancedPosition.vert";
		shader_info.stage = SHADER_STAGE_VERTEX;
		vertex_shader = Resources::createShader(shader_info);

		std::vector<VertexAttributeInfo> attribute_infos;
		//vertex buffer
		attribute_infos.emplace_back();
		attribute_infos[0].location = 0;
		attribute_infos[0].size = sizeof(vec3);
		//instance buffer
		attribute_infos.emplace_back();
		attribute_infos[1].location = 1;
		attribute_infos[1].size = sizeof(mat4);
		attribute_infos[1].flags = VERTEX_ATTRIBUTE_FLAG_PER_INSTANCE; //mark as instance buffer

		RasterizationPipelineInfo pipeline_info{};
		pipeline_info.flags = RASTERIZATION_PIPELINE_FLAG_CULL_BACK;
		pipeline_info.fragment_shader = fragment_shader;
		pipeline_info.vertex_shader = vertex_shader;
		pipeline_info.attribute_infos = attribute_infos.data();
		pipeline_info.attribute_info_count = attribute_infos.size();

		pipeline = Resources::createRasterizationPipeline(pipeline_info);

		RasterizationPipelineInstanceInfo pipeline_instance_info{};
		pipeline_instance_info.rasterization_pipeline = pipeline;
		pipeline_instance = Resources::createRasterizationPipelineInstance(pipeline_instance_info);

		MeshInfo mesh_info{};
		mesh_info.attribute_infos = attribute_infos.data();
		mesh_info.attribute_info_count = attribute_infos.size();

		cube_mesh = Resources::createMesh(mesh_info);
		Geometry::createCube(*cube_mesh, 1, 1, 1, VERTEX_FLAG_NONE);

		//Create instance buffer
		instance_transforms = new mat4[num_instances];
		for (int i = 0; i < num_instances; ++i) {
			instance_transforms[i] = glm::translate(mat4(1),
			                                        vec3(Random::floatRange(-range, range),
			                                             Random::floatRange(-range, range),
			                                             Random::floatRange(-range, (-range * 2))));
			instance_transforms[i] = glm::rotate(instance_transforms[i], Random::floatRange(0, 360), vec3(1, 0, 0));
			instance_transforms[i] = glm::rotate(instance_transforms[i], Random::floatRange(0, 360), vec3(0, 1, 0));
			instance_transforms[i] = glm::rotate(instance_transforms[i], Random::floatRange(0, 360), vec3(0, 0, 1));
		}

		cube_mesh->setInstanceBuffer(1, instance_transforms, num_instances);
	}

	void setupScene() {
		Entity cube_renderer_entity = createEntity3D();

		//todo: create an instancedMeshRenderer component
		MeshRenderer *cube_renderer = cube_renderer_entity.attach<MeshRenderer>();
		cube_renderer->use_transform_matrix_as_push_constant = false; // we are providing our transform in the form of an instance buffer
		cube_renderer->mesh = cube_mesh;
		cube_renderer->pipeline_instance = pipeline_instance;

		Entity camera_entity = createEntity3D();
		camera_entity.attach<Camera>();
		camera_entity.attach<CameraController>();
	}
};