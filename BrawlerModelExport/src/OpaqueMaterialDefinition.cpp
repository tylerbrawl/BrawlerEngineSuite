module;
#include <string>
#include <stdexcept>
#include <cassert>
#include <format>
#include <assimp/material.h>
#include "DxDef.h"

module Brawler.OpaqueMaterialDefinition;
import Brawler.MaterialIDMap;
import Brawler.ModelTextureDatabase;

namespace
{
	template <aiTextureType TextureType>
	struct TextureTypeStringInfo
	{
		static_assert(sizeof(TextureType) != sizeof(TextureType));
	};

	template <>
	struct TextureTypeStringInfo<aiTextureType::aiTextureType_DIFFUSE>
	{
		static constexpr std::string_view TEXTURE_TYPE_STR{ "diffuse albedo" };
	};
	
	template <aiTextureType TextureType>
	aiString GetTextureNameString(const aiMaterial& material, const std::uint32_t textureIndex = 0)
	{
		if (material.GetTextureCount(TextureType) < (textureIndex + 1)) [[unlikely]]
		{
			// For some reason, aiMaterial.GetName() is not const, even though it says that it is
			// in the documentation.
			aiString materialName{};
			const aiReturn materialNameResult{ material.Get(AI_MATKEY_NAME, materialName) };
			assert(materialNameResult == aiReturn::aiReturn_SUCCESS);

			throw std::runtime_error{ std::format("ERROR: The material {} does not have enough {} textures for an OpaqueMaterialDefinition!", materialName.C_Str(), TextureTypeStringInfo<TextureType>::TEXTURE_TYPE_STR.data()) };
		}

		aiString textureName{};
		const aiReturn textureNameResult{ material.GetTexture(TextureType, textureIndex, &textureName) };
		assert(textureNameResult == aiReturn::aiReturn_SUCCESS);

		return textureName;
	}
}

namespace Brawler
{
	OpaqueMaterialDefinition::OpaqueMaterialDefinition(const aiMaterial& material) :
		I_MaterialDefinition(),
		mDiffuseTextureContainer(ModelTextureContainer<aiTextureType::aiTextureType_DIFFUSE>{
			.ModelTexturePtr = nullptr,
			.ModelTextureName{GetTextureNameString<aiTextureType::aiTextureType_DIFFUSE>(material)}
		})
	{
		// For now, we will only worry about the diffuse albedo texture.

		
	}

	void OpaqueMaterialDefinition::RegisterModelTextures(ModelTextureDatabase& textureDatabase)
	{

	}

	MaterialID OpaqueMaterialDefinition::GetMaterialID() const
	{
		return Brawler::GetMaterialID<OpaqueMaterialDefinition>();
	}
}