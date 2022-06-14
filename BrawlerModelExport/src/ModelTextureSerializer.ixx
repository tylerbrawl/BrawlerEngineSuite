module;
#include <format>
#include <DirectXTex.h>

export module Brawler.ModelTextureSerializer;
import Brawler.ImportedMesh;
import Brawler.ModelTextureID;
import Brawler.TextureTypeMap;
import Brawler.FilePathHash;
import Brawler.NZStringView;

export namespace Brawler
{
	template <ModelTextureID TextureType>
	class ModelTextureSerializer
	{
	public:
		ModelTextureSerializer(const ImportedMesh& mesh, const DirectX::ScratchImage& finalizedModelTexture);

		ModelTextureSerializer(const ModelTextureSerializer& rhs) = delete;
		ModelTextureSerializer& operator=(const ModelTextureSerializer& rhs) = delete;

		ModelTextureSerializer(ModelTextureSerializer&& rhs) noexcept = delete;
		ModelTextureSerializer& operator=(ModelTextureSerializer&& rhs) noexcept = delete;

		FilePathHash GetModelTextureFilePathHash() const;

	private:
		FilePathHash mSerializedFilePathHash;
	};
}

// ------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	FilePathHash SerializeModelTexture(const Brawler::NZWStringView textureName, const DirectX::ScratchImage& finalizedModelTexture);
}

namespace Brawler
{
	template <ModelTextureID TextureType>
	ModelTextureSerializer<TextureType>::ModelTextureSerializer(const ImportedMesh& mesh, const DirectX::ScratchImage& finalizedModelTexture) :
		mSerializedFilePathHash()
	{
		constexpr NZWStringView TEXTURE_NAME_FORMAT_STR{ Brawler::GetTextureNameFormatString<TextureType>() };
		mSerializedFilePathHash = SerializeModelTexture(std::format(TEXTURE_NAME_FORMAT_STR.C_Str(), mesh.GetLODScene().GetLODLevel(), mesh.GetMeshIDForLOD()), finalizedModelTexture);
	}

	template <ModelTextureID TextureType>
	FilePathHash ModelTextureSerializer<TextureType>::GetModelTextureFilePathHash() const
	{
		return mSerializedFilePathHash;
	}
}