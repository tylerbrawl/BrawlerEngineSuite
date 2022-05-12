module;
#include <span>
#include <DirectXTex.h>

export module Brawler.MipMapGeneration:GenericMipMapGenerator;
import Util.General;

export namespace Brawler
{
	class GenericMipMapGenerator
	{
	public:
		GenericMipMapGenerator() = default;

		GenericMipMapGenerator(const GenericMipMapGenerator& rhs) = delete;
		GenericMipMapGenerator& operator=(const GenericMipMapGenerator& rhs) = delete;

		GenericMipMapGenerator(GenericMipMapGenerator&& rhs) noexcept = default;
		GenericMipMapGenerator& operator=(GenericMipMapGenerator&& rhs) noexcept = default;

		void BeginMipMapGeneration(const DirectX::ScratchImage& srcTexture);
		bool IsMipMapGenerationFinished() const;

		DirectX::ScratchImage ExtractGeneratedMipMaps();

	private:
		DirectX::ScratchImage mGeneratedMipMapTexture;
	};
}

// -----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	void GenericMipMapGenerator::BeginMipMapGeneration(const DirectX::ScratchImage& srcTexture)
	{
		// DirectXTex does not support generating mip-map chains directly from BC formats.
		// We could first decompress the textures, create the chain, and then compress them 
		// again, but this can be incredibly slow.

		if constexpr (Util::General::IsDebugModeEnabled())
		{
			const std::span<const DirectX::Image> imageSpan{ srcTexture.GetImages(), srcTexture.GetImageCount() };

			for (const auto& image : imageSpan)
				assert(!DirectX::IsCompressed(image.format) && "ERROR: A block-compressed image was provided for GenericMipMapGenerator::BeginMipMapGeneration()! (Did you forget to use Util::ModelTexture::CreateIntermediateTexture()?)");
		}

		Util::General::CheckHRESULT(DirectX::GenerateMipMaps(
			srcTexture.GetImages(),
			srcTexture.GetImageCount(),
			srcTexture.GetMetadata(),
			DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
			0,  // Create a full mip-map chain, down to a 1x1 texture.
			mGeneratedMipMapTexture
		));
	}

	bool GenericMipMapGenerator::IsMipMapGenerationFinished() const
	{
		// The GenericMipMapGenerator does all of its work immediately on the CPU timeline, so
		// this function always returns true.

		return true;
	}

	DirectX::ScratchImage GenericMipMapGenerator::ExtractGeneratedMipMaps()
	{
		return std::move(mGeneratedMipMapTexture);
	}
}