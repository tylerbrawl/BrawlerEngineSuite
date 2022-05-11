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
	public:
		explicit OpaqueMaterialDefinition(const aiMaterial& material);

		OpaqueMaterialDefinition(const OpaqueMaterialDefinition& rhs) = delete;
		OpaqueMaterialDefinition& operator=(const OpaqueMaterialDefinition& rhs) = delete;

		OpaqueMaterialDefinition(OpaqueMaterialDefinition&& rhs) noexcept = default;
		OpaqueMaterialDefinition& operator=(OpaqueMaterialDefinition&& rhs) noexcept = default;

		MaterialID GetMaterialID() const override;

	private:
		ModelTexture<aiTextureType::aiTextureType_DIFFUSE> mDiffuseTexture;
	};
}