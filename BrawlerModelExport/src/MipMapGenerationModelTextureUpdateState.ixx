module;
#include <optional>
#include <assimp/material.h>
#include <DirectXTex.h>

export module Brawler.MipMapGenerationModelTextureUpdateState;
import Brawler.I_ModelTextureUpdateState;
import Brawler.FormatConversionModelTextureUpdateState;
import Brawler.TextureTypeMap;

export namespace Brawler
{
	template <aiTextureType TextureType>
	class MipMapGenerationModelTextureUpdateState final : public I_ModelTextureUpdateState<MipMapGenerationModelTextureUpdateState>, private ModelTextureMipMapGeneratorType<TextureType>
	{
	public:
		MipMapGenerationModelTextureUpdateState() = default;

		MipMapGenerationModelTextureUpdateState(const MipMapGenerationModelTextureUpdateState& rhs) = delete;
		MipMapGenerationModelTextureUpdateState& operator=(const MipMapGenerationModelTextureUpdateState& rhs) = delete;

		MipMapGenerationModelTextureUpdateState(MipMapGenerationModelTextureUpdateState&& rhs) noexcept = default;
		MipMapGenerationModelTextureUpdateState& operator=(MipMapGenerationModelTextureUpdateState&& rhs) noexcept = default;

		void UpdateTextureScratchImage(DirectX::ScratchImage& image);
		bool IsFinalTextureReadyForSerialization() const;

		std::optional<FormatConversionModelTextureUpdateState> GetNextState() const;
	};
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
	void MipMapGenerationModelTextureUpdateState<TextureType>::UpdateTextureScratchImage(DirectX::ScratchImage& image)
	{
		ModelTextureMipMapGeneratorType<TextureType>::Update(image);

		if (ModelTextureMipMapGeneratorType<TextureType>::IsMipMapGenerationFinished())
			image = ModelTextureMipMapGeneratorType<TextureType>::ExtractGeneratedMipMaps();
	}

	template <aiTextureType TextureType>
	bool MipMapGenerationModelTextureUpdateState<TextureType>::IsFinalTextureReadyForSerialization() const
	{
		return false;
	}

	template <aiTextureType TextureType>
	std::optional<FormatConversionModelTextureUpdateState> MipMapGenerationModelTextureUpdateState<TextureType>::GetNextState() const
	{
		return (ModelTextureMipMapGeneratorType<TextureType>::IsMipMapGenerationFinished() ? FormatConversionModelTextureUpdateState{} : std::optional<FormatConversionModelTextureUpdateState>{});
	}
}