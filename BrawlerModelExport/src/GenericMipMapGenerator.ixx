module;
#include <span>
#include <optional>
#include <DirectXTex.h>

export module Brawler.MipMapGeneration:GenericMipMapGenerator;
import Util.General;
import Util.Win32;
import Brawler.Win32.ConsoleFormat;

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

		void Update(const DirectX::ScratchImage& srcTexture);
		bool IsMipMapGenerationFinished() const;

		DirectX::ScratchImage ExtractGeneratedMipMaps();

	private:
		std::optional<DirectX::ScratchImage> mGeneratedMipMapTexture;
	};
}

// -----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	void GenericMipMapGenerator::Update(const DirectX::ScratchImage& srcTexture)
	{
		// DirectXTex does not support generating mip-map chains directly from BC formats.
		// We could first decompress the textures, create the chain, and then compress them 
		// again, but this can be incredibly slow.

		if (!mGeneratedMipMapTexture.has_value()) [[likely]]
		{
			DirectX::ScratchImage generatedTexture{};
			
			if constexpr (Util::General::IsDebugModeEnabled())
			{
				const std::span<const DirectX::Image> imageSpan{ srcTexture.GetImages(), srcTexture.GetImageCount() };

				for (const auto& image : imageSpan)
					assert(!DirectX::IsCompressed(image.format) && "ERROR: A block-compressed image was provided for GenericMipMapGenerator::BeginMipMapGeneration()! (Did you forget to use Util::ModelTexture::CreateIntermediateTexture()?)");
			}

			const HRESULT hr = DirectX::GenerateMipMaps(
				srcTexture.GetImages(),
				srcTexture.GetImageCount(),
				srcTexture.GetMetadata(),
				DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
				0,  // Create a full mip-map chain, down to a 1x1 texture.
				generatedTexture
			);

			if (FAILED(hr)) [[unlikely]]
			{
				// If mip-map generation failed, then send out a warning, but only if the texture should
				// supposedly have the ability to generate mip-maps.

				const std::span<const DirectX::Image> imageSpan{ srcTexture.GetImages(), srcTexture.GetImageCount() };
				bool issueWarning = true;

				for (const auto& image : imageSpan)
				{
					if (image.width == image.height && image.width == 1)
						issueWarning = false;
				}

				if (issueWarning) [[unlikely]]
					Util::Win32::WriteFormattedConsoleMessage(L"WARNING: Mip-map generation for a ModelTexture failed, even though it probably shouldn't have!", Brawler::Win32::ConsoleFormat::WARNING);

				generatedTexture.Initialize(srcTexture.GetMetadata());

				const std::span<const DirectX::Image> destImageSpan{ generatedTexture.GetImages(), generatedTexture.GetImageCount() };
				assert(destImageSpan.size() == imageSpan.size());
				
				for (std::size_t i = 0; i < destImageSpan.size(); ++i)
					std::memcpy(destImageSpan[i].pixels, imageSpan[i].pixels, destImageSpan[i].slicePitch);
			}

			mGeneratedMipMapTexture = std::move(generatedTexture);
		}
	}

	bool GenericMipMapGenerator::IsMipMapGenerationFinished() const
	{
		// The GenericMipMapGenerator does all of its work immediately on the CPU timeline, so
		// this function always returns true.

		return true;
	}

	DirectX::ScratchImage GenericMipMapGenerator::ExtractGeneratedMipMaps()
	{
		return std::move(*mGeneratedMipMapTexture);
	}
}