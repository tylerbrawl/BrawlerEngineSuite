module;
#include <string>
#include <stdexcept>
#include <cassert>
#include <format>
#include <optional>
#include <memory>
#include <assimp/material.h>
#include "DxDef.h"

module Brawler.OpaqueMaterialDefinition;
import Brawler.MaterialIDMap;
import Brawler.ModelTextureDatabase;
import Brawler.AssimpMaterialKey;
import Brawler.MaterialColorModelTextureBuilder;
import Util.ModelTexture;

namespace
{
	aiString GetMaterialName(const aiMaterial& material)
	{
		aiString materialName{};
		const aiReturn materialNameResult{ material.Get(AI_MATKEY_NAME, materialName) };

		assert(materialNameResult == aiReturn::aiReturn_SUCCESS);

		return materialName;
	}
}

namespace Brawler
{
	OpaqueMaterialDefinition::OpaqueMaterialDefinition(ImportedMesh&& mesh) :
		I_MaterialDefinition(std::move(mesh)),
		mHDiffuseTexture()
	{}

	ModelTextureBuilderCollection OpaqueMaterialDefinition::CreateModelTextureBuilders()
	{
		// For now, we will only worry about the diffuse albedo texture.
		
		ModelTextureBuilderCollection textureBuilderCollection{};
		std::unique_ptr<I_ModelTextureBuilder<aiTextureType::aiTextureType_DIFFUSE>> diffuseTextureBuilder{};

		const aiMaterial& material{ GetImportedMesh().GetMeshMaterial() };

		const std::uint32_t diffuseTextureCount = material.GetTextureCount(aiTextureType::aiTextureType_DIFFUSE);

		if (diffuseTextureCount < 1) [[unlikely]]
		{
			// If the material has no diffuse texture, then we try to make one from its diffuse color.
			const std::optional<aiColor3D> diffuseColor{ Brawler::GetAssimpMaterialProperty<AssimpMaterialKeyID::COLOR_DIFFUSE>(material) };

			if (!diffuseColor.has_value()) [[unlikely]]
				throw std::runtime_error{ std::format("ERROR: The material {} has neither enough diffuse albedo textures nor a diffuse albedo material color for an OpaqueMaterialDefinition!", GetMaterialName(material).C_Str()) };

			diffuseTextureBuilder = std::make_unique<MaterialColorModelTextureBuilder<aiTextureType::aiTextureType_DIFFUSE>>(GetImportedMesh());
		}
		else
		{
			std::optional<std::unique_ptr<I_ModelTextureBuilder<aiTextureType::aiTextureType_DIFFUSE>>> optionalDiffuseTextureBuilder{ Util::ModelTexture::CreateModelTextureBuilderForExistingTexture<aiTextureType::aiTextureType_DIFFUSE>(GetImportedMesh()) };
			assert(optionalDiffuseTextureBuilder.has_value());

			diffuseTextureBuilder = std::move(*optionalDiffuseTextureBuilder);
		}

		mHDiffuseTexture = textureBuilderCollection.AddModelTextureBuilder(std::move(diffuseTextureBuilder));
		return textureBuilderCollection;
	}

	void OpaqueMaterialDefinition::Update()
	{}

	bool OpaqueMaterialDefinition::IsReadyForSerialization() const
	{
		// Serializing the OpaqueMaterialDefinition involves writing out the output FilePathHash
		// of all involved textures. These are ready by the time the corresponding
		// ModelTextureHandle instances are created.
		//
		// So, we have all of the data needed to serialize this definition.

		return true;
	}

	MaterialID OpaqueMaterialDefinition::GetMaterialID() const
	{
		return Brawler::GetMaterialID<OpaqueMaterialDefinition>();
	}
}