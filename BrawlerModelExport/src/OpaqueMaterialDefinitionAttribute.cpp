module;
#include <string>
#include <cassert>
#include <stdexcept>
#include <assimp/scene.h>
#include <DirectXTex.h>

module Brawler.OpaqueMaterialDefinitionAttribute;
import Brawler.SerializationContext;
import Util.Texture;
import Brawler.SerializedMeshAttribute;
import Brawler.FilePathHash;

namespace Brawler
{
	OpaqueMaterialDefinitionAttribute::OpaqueMaterialDefinitionAttribute(const aiMaterial& material) :
		I_MeshAttribute(),
		mDiffuseTexture()
	{
		// For now, we will only worry about the diffuse albedo texture.

		if (material.GetTextureCount(aiTextureType_DIFFUSE) == 0) [[unlikely]]
		{
			// For some reason, aiMaterial.GetName() is not const, even though it says that it is
			// in the documentation.
			aiString materialName{};
			assert(material.Get(AI_MATKEY_NAME, materialName) == aiReturn_SUCCESS);

			std::string errMsg{ "ERROR: The material " + std::string{ materialName.C_Str() } + " does not have a diffuse albedo texture!" };
			throw std::runtime_error{ std::move(errMsg) };
		}

		aiString diffuseTextureName{};
		const aiReturn diffuseResult{ material.GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTextureName) };
		assert(diffuseResult == aiReturn_SUCCESS);

		mDiffuseTexture = Texture<aiTextureType::aiTextureType_DIFFUSE>{ diffuseTextureName };
		mDiffuseTexture.GenerateMipMaps();
	}

	std::vector<std::uint8_t> OpaqueMaterialDefinitionAttribute::SerializeAttributeData(const SerializationContext& context) const
	{
		WriteConvertedTextures();

		const SerializedMeshAttribute<OpaqueMaterialDefinitionAttribute> serializedAttribute{
			.AlbedoTexturePathHash = mDiffuseTexture.GetOutputPathHash().GetHash()
		};

		std::vector<std::uint8_t> byteArr{};
		byteArr.resize(sizeof(serializedAttribute));

		std::memcpy(byteArr.data(), &serializedAttribute, sizeof(serializedAttribute));

		return byteArr;
	}

	void OpaqueMaterialDefinitionAttribute::WriteConvertedTextures() const
	{
		// For now, only diffuse textures are supported. For opaque materials, these will be
		// in the BC7_UNORM_SRGB format.
		
		mDiffuseTexture.WriteToFileSystem();
	}
}