module;
#include <assimp/material.h>

export module Brawler.OpaqueMaterialDefinition;
import Brawler.I_MaterialDefinition;
import Brawler.MaterialID;
import Brawler.ModelTexture;

export namespace Brawler
{
	class OpaqueMaterialDefinition final : public I_MaterialDefinition
	{
	private:
		template <aiTextureType TextureType>
		struct ModelTextureContainer
		{
			ModelTexture<TextureType>* ModelTexturePtr;
			aiString ModelTextureName;
		};

	public:
		explicit OpaqueMaterialDefinition(const aiMaterial& material);

		OpaqueMaterialDefinition(const OpaqueMaterialDefinition& rhs) = delete;
		OpaqueMaterialDefinition& operator=(const OpaqueMaterialDefinition& rhs) = delete;

		OpaqueMaterialDefinition(OpaqueMaterialDefinition&& rhs) noexcept = default;
		OpaqueMaterialDefinition& operator=(OpaqueMaterialDefinition&& rhs) noexcept = default;

		void RegisterModelTextures(ModelTextureDatabase& textureDatabase) override;

		MaterialID GetMaterialID() const override;

	private:
		ModelTextureContainer<aiTextureType::aiTextureType_DIFFUSE> mDiffuseTextureContainer;
	};
}