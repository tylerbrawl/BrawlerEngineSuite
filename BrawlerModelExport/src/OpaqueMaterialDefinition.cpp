module;
#include <string>
#include <stdexcept>
#include <cassert>
#include <assimp/material.h>
#include "DxDef.h"

module Brawler.OpaqueMaterialDefinition;
import Brawler.MaterialIDMap;

namespace Brawler
{
	OpaqueMaterialDefinition::OpaqueMaterialDefinition(const aiMaterial& material) :
		mDiffuseTexture()
	{
		// For now, we will only worry about the diffuse albedo texture.

		if (material.GetTextureCount(aiTextureType_DIFFUSE) == 0) [[unlikely]]
		{
			// For some reason, aiMaterial.GetName() is not const, even though it says that it is
			// in the documentation.
			aiString materialName{};
			const aiReturn materialNameResult{ material.Get(AI_MATKEY_NAME, materialName) };
			assert(materialNameResult == aiReturn_SUCCESS);

			std::string errMsg{ "ERROR: The material " + std::string{ materialName.C_Str() } + " does not have a diffuse albedo texture!" };
			throw std::runtime_error{ std::move(errMsg) };
		}

		aiString diffuseTextureName{};
		const aiReturn diffuseResult{ material.GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTextureName) };
		assert(diffuseResult == aiReturn_SUCCESS);

		mDiffuseTexture = Texture<aiTextureType::aiTextureType_DIFFUSE>{ diffuseTextureName };
		mDiffuseTexture.GenerateMipMaps();

		mDiffuseTexture.WriteToFileSystem();
	}

	MaterialID OpaqueMaterialDefinition::GetMaterialID() const
	{
		return Brawler::GetMaterialID<OpaqueMaterialDefinition>();
	}
}