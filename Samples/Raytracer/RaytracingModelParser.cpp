
#include "RaytracingModelParser.h"

RaytracingModelParser::RaytracingModelParser(const RaytracingModelParserInfo &info) : HBE::DefaultModelParser({MESH_FLAG_GENERATE_ATTRIBUTE_STORAGE_BUFFER}) {
	this->info = info;
}

HBE::Mesh *RaytracingModelParser::createMesh(const HBE::ModelPrimitiveData &data, HBE::ModelInfo model_info) {
	Mesh *mesh = DefaultModelParser::createMesh(data, model_info);
	info.meshes->push_back(mesh);
	return mesh;
}

HBE::RasterizationPipelineInstance *RaytracingModelParser::createMaterial(const HBE::ModelMaterialData &materialData, HBE::Image **textures) {
	MaterialData material;
	material.albedo = materialData.properties.base_color;
	material.emission = materialData.properties.emmisive_factor;
	material.roughness = materialData.properties.roughness;
	if (materialData.properties.has_albedo)
		material.albedo_texture_index = texture_index_offset + materialData.albedo_texture;
	if (materialData.properties.has_normal)
		material.normal_texture_index = texture_index_offset + materialData.normal_texture;

	info.materials->push_back(material);
	return nullptr; //graphic pipeline is unused
}

HBE::Image *RaytracingModelParser::createTexture(const HBE::ModelTextureData &data) {
	Image *texture = DefaultModelParser::createTexture(data);
	info.textures->push_back(texture);
	return texture;

}

MeshAccelerationStructure *RaytracingModelParser::createMeshAccelerationStructure(Mesh *mesh, int mesh_index) {
	if (mesh_to_acceleration_structure_index.find(mesh_index) != mesh_to_acceleration_structure_index.end()) {
		mesh_to_acceleration_structure_index.emplace(mesh_index, info.acceleration_structures->size());
	}
	MeshAccelerationStructure *meshAccelerationStructure = DefaultModelParser::createMeshAccelerationStructure(mesh, mesh_index);
	info.acceleration_structures->emplace_back(meshAccelerationStructure);
	return meshAccelerationStructure;
}

void RaytracingModelParser::onStartParsingModel(HBE::Model *model) {
	mesh_to_acceleration_structure_index.clear();
	material_index_offset = info.materials->size();
	texture_index_offset = info.textures->size();
}

AccelerationStructureInstance RaytracingModelParser::createAccelerationStructureInstance(ModelNode &node, int primitive) {
	AccelerationStructureInstance instance = {};
	instance.acceleration_structure_index = mesh_to_acceleration_structure_index[node.mesh] + primitive;
	instance.transform = node.transform;
	instance.shader_group_index = info.mesh_shader_group_index;
	instance.custom_index = material_index_offset + node.primitives[primitive].material;
	return instance;
}






