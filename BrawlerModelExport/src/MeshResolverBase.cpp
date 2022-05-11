module;
#include <memory>
#include <format>
#include <assimp/mesh.h>

module Brawler.MeshResolverBase;

namespace
{
	std::unique_ptr<Brawler::I_MaterialDefinition> CreateMaterialDefinition(const aiMaterial& material)
	{
		aiString materialName{};
		const aiReturn getMaterialNameReturn{ material.Get(AI_MATKEY_NAME, materialName) };
		assert(getMaterialNameReturn == aiReturn::aiReturn_SUCCESS);

		// For now, we only support opaque materials.

		float materialOpacity = 0.0f;
		const aiReturn getMaterialOpacityReturn{ material.Get(AI_MATKEY_OPACITY, materialOpacity) };
		assert(getMaterialOpacityReturn == aiReturn::aiReturn_SUCCESS);

		if (materialOpacity != 1.0f) [[unlikely]]
			throw std::runtime_error{ std::format("ERROR: The material {} is transparent! (Currently, transparent/translucent materials are not supported.)", materialName.C_Str()) };

		return std::make_unique<Brawler::OpaqueMaterialDefinition>(material);
	}
}

namespace Brawler
{
	MeshResolverBase::MeshResolverBase(ImportedMesh&& mesh) :
		mMaterialDefinitionPtr(CreateMaterialDefinition(mesh.GetMeshMaterial())),
		mImportedMesh(std::move(mesh))
	{}

	const ImportedMesh& MeshResolverBase::GetImportedMesh() const
	{
		return mImportedMesh;
	}
}