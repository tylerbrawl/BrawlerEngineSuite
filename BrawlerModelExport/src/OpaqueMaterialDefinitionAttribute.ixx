module;
#include <vector>
#include <assimp/material.h>

export module Brawler.OpaqueMaterialDefinitionAttribute;
import Brawler.I_MeshAttribute;
import Brawler.Texture;

export namespace Brawler
{
	class OpaqueMaterialDefinitionAttribute final : public I_MeshAttribute
	{
	public:
		explicit OpaqueMaterialDefinitionAttribute(const aiMaterial& material);

		std::vector<std::uint8_t> SerializeAttributeData(const SerializationContext& context) const override;

	private:
		void WriteConvertedTextures() const;

	private:
		Texture<aiTextureType::aiTextureType_DIFFUSE> mDiffuseTexture;
	};
}