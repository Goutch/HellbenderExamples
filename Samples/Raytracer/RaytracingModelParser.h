
#pragma once

#include "HBE.h"
#include "DataStructures.h"

struct RaytracingModelParserInfo {
	int mesh_shader_group_index;
	std::vector<HBE::Image *> *textures;
	std::vector<MaterialData> *materials;
	std::vector<Mesh *> *meshes;
	std::vector<MeshAccelerationStructure *> *acceleration_structures;
};

class RaytracingModelParser : public HBE::DefaultModelParser {
	RaytracingModelParserInfo info;
	std::map<int, int> mesh_to_acceleration_structure_index;
	int material_index_offset = 0;
	int texture_index_offset = 0;
public:
	RaytracingModelParser(const RaytracingModelParserInfo &info);

	HBE::Mesh *createMesh(const HBE::ModelPrimitiveData &data, HBE::ModelInfo model_info) override;

	HBE::RasterizationPipelineInstance *createMaterial(const HBE::ModelMaterialData &materialData, HBE::Image **textures) override;

	Image *createTexture(const ModelTextureData &data) override;

	void onStartParsingModel(HBE::Model *model) override;

	MeshAccelerationStructure *createMeshAccelerationStructure(Mesh *mesh, int mesh_index) override;

private:
	AccelerationStructureInstance createAccelerationStructureInstance(ModelNode &node, int primitive) override;

};
