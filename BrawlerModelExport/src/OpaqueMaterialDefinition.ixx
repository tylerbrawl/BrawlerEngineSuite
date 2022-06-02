module;
#include <assimp/material.h>

export module Brawler.OpaqueMaterialDefinition;
import Brawler.I_MaterialDefinition;
import Brawler.MaterialID;
import Brawler.OpaqueDiffuseModelTextureResolver;

export namespace Brawler
{
	class OpaqueMaterialDefinition final : public I_MaterialDefinition
	{
	public:
		explicit OpaqueMaterialDefinition(ImportedMesh&& mesh);

		OpaqueMaterialDefinition(const OpaqueMaterialDefinition& rhs) = delete;
		OpaqueMaterialDefinition& operator=(const OpaqueMaterialDefinition& rhs) = delete;

		OpaqueMaterialDefinition(OpaqueMaterialDefinition&& rhs) noexcept = default;
		OpaqueMaterialDefinition& operator=(OpaqueMaterialDefinition&& rhs) noexcept = default;

		void Update() override;
		bool IsReadyForSerialization() const override;

		MaterialID GetMaterialID() const override;

	private:
		OpaqueDiffuseModelTextureResolver mDiffuseTextureResolver;
	};
}