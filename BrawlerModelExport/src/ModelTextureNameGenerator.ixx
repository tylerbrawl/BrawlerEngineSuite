module;
#include <string>
#include <format>

export module Brawler.ModelTextureNameGenerator;
import Brawler.ModelTextureID;
import Brawler.TextureTypeMap;
import Brawler.ImportedMesh;
import Brawler.LODScene;
import Brawler.NZStringView;

export namespace Brawler
{
	template <ModelTextureID TextureID>
	class ModelTextureNameGenerator
	{
	public:
		ModelTextureNameGenerator() = default;
		explicit ModelTextureNameGenerator(const ImportedMesh& mesh);

		ModelTextureNameGenerator(const ModelTextureNameGenerator& rhs) = delete;
		ModelTextureNameGenerator& operator=(const ModelTextureNameGenerator& rhs) = delete;

		ModelTextureNameGenerator(ModelTextureNameGenerator&& rhs) noexcept = default;
		ModelTextureNameGenerator& operator=(ModelTextureNameGenerator&& rhs) noexcept = default;

		const NZWStringView GetModelTextureName() const;

	private:
		std::wstring mModelTextureNameStr;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <ModelTextureID TextureID>
	ModelTextureNameGenerator<TextureID>::ModelTextureNameGenerator(const ImportedMesh& mesh) :
		mModelTextureNameStr()
	{
		constexpr NZWStringView TEXTURE_NAME_FORMAT_STR{ GetTextureNameFormatString<TextureID>() };

		mModelTextureNameStr = std::format(TEXTURE_NAME_FORMAT_STR, mesh.GetLODScene().GetLODLevel(), mesh.GetMeshIDForLOD());
	}

	template <ModelTextureID TextureID>
	const NZWStringView ModelTextureNameGenerator<TextureID>::GetModelTextureName() const
	{
		return NZWStringView{ mModelTextureNameStr };
	}
}