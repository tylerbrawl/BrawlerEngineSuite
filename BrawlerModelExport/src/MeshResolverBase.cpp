module;
#include <memory>
#include <format>
#include <cassert>
#include <assimp/scene.h>

module Brawler.MeshResolverBase;
import Brawler.OpaqueMaterialDefinition;

namespace Brawler
{
	std::unique_ptr<Brawler::I_MaterialDefinition> CreateMaterialDefinition(const Brawler::ImportedMesh& mesh)
	{
		const aiMaterial& material{ mesh.GetMeshMaterial() };

		aiString materialName{};
		const aiReturn getMaterialNameReturn{ material.Get(AI_MATKEY_NAME, materialName) };
		assert(getMaterialNameReturn == aiReturn::aiReturn_SUCCESS);

		// For now, we only support opaque materials.

		float materialOpacity = 0.0f;
		const aiReturn getMaterialOpacityReturn{ material.Get(AI_MATKEY_OPACITY, materialOpacity) };
		assert(getMaterialOpacityReturn == aiReturn::aiReturn_SUCCESS);

		if (materialOpacity != 1.0f) [[unlikely]]
			throw std::runtime_error{ std::format("ERROR: The material {} is transparent! (Currently, transparent/translucent materials are not supported.)", materialName.C_Str()) };

		return std::make_unique<Brawler::OpaqueMaterialDefinition>(Brawler::ImportedMesh{ mesh });
	}
}