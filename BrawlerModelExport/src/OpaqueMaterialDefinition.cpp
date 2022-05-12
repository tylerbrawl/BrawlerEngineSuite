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
		mHDiffuseTexture(),
		mInitialized(nullptr)
	{}

	void OpaqueMaterialDefinition::Update()
	{
		if (!mInitialized) [[unlikely]]
		{
			VerifyModelTextureCount();
			CreateModelTextureHandles();

			mInitialized = true;
		}
	}

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

	void OpaqueMaterialDefinition::VerifyModelTextureCount() const
	{
		// For now, we will only worry about the diffuse albedo texture.
		
		const aiMaterial& material{ GetImportedMesh().GetMeshMaterial() };

		{
			const std::uint32_t diffuseTextureCount = material.GetTextureCount(aiTextureType::aiTextureType_DIFFUSE);

			if (diffuseTextureCount < 1) [[unlikely]]
				throw std::runtime_error{ std::format("ERROR: The material {} does not have enough diffuse albedo textures for an OpaqueMaterialDefinition!", GetMaterialName(material).C_Str()) };
		}
	}

	void OpaqueMaterialDefinition::CreateModelTextureHandles()
	{
		mHDiffuseTexture = ModelTextureDatabase::GetInstance().GetModelTextureHandle<aiTextureType::aiTextureType_DIFFUSE>(GetImportedMesh());
	}
}