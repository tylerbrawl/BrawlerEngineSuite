module;
#include <optional>
#include <filesystem>
#include <cassert>
#include <string_view>
#include <format>
#include <assimp/scene.h>
#include <DirectXTex.h>

export module Brawler.EmbeddedTextureModelTextureBuilder;
import Brawler.I_ModelTextureBuilder;
import Brawler.LODScene;
import Util.General;
import Util.Win32;
import Brawler.Win32.ConsoleFormat;
import Brawler.TextureTypeMap;

export namespace Brawler
{
	template <aiTextureType TextureType>
	class EmbeddedTextureModelTextureBuilder final : public I_ModelTextureBuilder<TextureType>
	{
	public:
		EmbeddedTextureModelTextureBuilder(const LODScene& scene, const aiString& textureName);

		EmbeddedTextureModelTextureBuilder(const EmbeddedTextureModelTextureBuilder& rhs) = delete;
		EmbeddedTextureModelTextureBuilder& operator=(const EmbeddedTextureModelTextureBuilder& rhs) = delete;

		EmbeddedTextureModelTextureBuilder(EmbeddedTextureModelTextureBuilder&& rhs) noexcept = default;
		EmbeddedTextureModelTextureBuilder& operator=(EmbeddedTextureModelTextureBuilder&& rhs) noexcept = default;

		DirectX::ScratchImage CreateIntermediateScratchTexture() const override;
		std::wstring_view GetUniqueTextureName() const override;

	private:
		DirectX::ScratchImage CreateIntermediateScratchTextureFromAssimpInterpretedEmbeddedTexture() const;
		DirectX::ScratchImage CreateIntermediateScratchTextureFromCompressedEmbeddedTexture() const;

	private:
		std::wstring mUniqueTextureName;
		const aiTexture* mTexturePtr;
	};
}

// ---------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
	EmbeddedTextureModelTextureBuilder<TextureType>::EmbeddedTextureModelTextureBuilder(const LODScene& scene, const aiString& textureName) :
		I_ModelTextureBuilder<TextureType>(),
		mUniqueTextureName(std::format(L"EmbeddedLOD{}_{}", scene.GetLODLevel(), Util::General::StringToWString(textureName.C_Str()))),
		mTexturePtr(scene.GetScene().GetEmbeddedTexture(textureName.C_Str()))
	{
		assert(mTexturePtr != nullptr && "ERROR: An EmbeddedTextureModelTextureBuilder instance was used to import an external texture file!");
	}

	template <aiTextureType TextureType>
	DirectX::ScratchImage EmbeddedTextureModelTextureBuilder<TextureType>::CreateIntermediateScratchTexture() const
	{
		// Assimp stores the size of the data in two ways, depending on whether or not the
		// texture data is compressed:
		//
		//   - If the texture is *NOT* compressed, then aiTexture.pcData contains the texture
		//     format in A8R8G8B8_UNORM format*. The size of this array is aiTexture.mWidth * 
		//     aiTexture.mHeight.
		//
		//   - If the texture *IS* compressed, then aiTexture.mHeight == 0 and aiTexture.pcData
		//     contains the raw texture data. This is the case for textures such as .png files.
		//
		// *At least, I'm pretty sure that it is in A8R8G8B8_UNORM format. Other parts of the
		// documentation state that it is in R8G8B8A8_UNORM format, though. This is almost as bad as
		// the barren wasteland of DirectX 12 documentation...

		return (mTexturePtr->mHeight == 0 ? CreateIntermediateScratchTextureFromCompressedEmbeddedTexture() : CreateIntermediateScratchTextureFromAssimpInterpretedEmbeddedTexture());
	}

	template <aiTextureType TextureType>
	std::wstring_view EmbeddedTextureModelTextureBuilder<TextureType>::GetUniqueTextureName() const
	{
		return mUniqueTextureName;
	}

	template <aiTextureType TextureType>
	DirectX::ScratchImage EmbeddedTextureModelTextureBuilder<TextureType>::CreateIntermediateScratchTextureFromAssimpInterpretedEmbeddedTexture() const
	{
		// mTexturePtr contains the actual texture data. So, we fill a DirectX::Image 
		// structure with the appropriate data and create the scratch texture with that.
		//
		// Unfortunately, A8R8G8B8_UNORM format is not a valid DXGI_FORMAT enumeration, so
		// we have to do some swizzling. To prevent potential errors, we need to create a copy 
		// of the texture data and swizzle that, instead.

		// Warn the user that this texture may have been processed incorrectly.
		Util::Win32::WriteFormattedConsoleMessage(std::string{ "WARNING: The embedded texture " } + mTexturePtr->mFilename.C_Str() + " was interpreted by Assimp ambiguously. The resulting texture file, if produced, may be incorrect for several reasons, such as color space inconsistency. Please use either unembedded textures or embedded textures in a compressed format, such as .png.", Brawler::Win32::ConsoleFormat::WARNING);

		assert(std::strcmp(mTexturePtr->achFormatHint, "argb8888") == 0 && "ERROR: Assimp's documentation was lying about its embedded texture format!");

		// Create a copy of the texture data for swizzling.
		const std::size_t textureSize = static_cast<std::size_t>(mTexturePtr->mWidth) * static_cast<std::size_t>(mTexturePtr->mHeight);
		std::vector<aiTexel> swizzledTexelArr{};
		swizzledTexelArr.resize(textureSize);

		std::memcpy(swizzledTexelArr.data(), mTexturePtr->pcData, textureSize * sizeof(aiTexel));

		for (auto& texel : swizzledTexelArr)
		{
			// This is the order in which the fields are listed in the aiTexel structure. The
			// names are misleading because texel data is not necessarily stored in this order.
			DirectX::XMVECTOR swizzleVector{ DirectX::XMVectorSet(texel.b, texel.g, texel.r, texel.a) };

			// Convert the texel from A8R8G8B8_UINT format to R8G8B8A8_UINT format.
			swizzleVector = DirectX::XMVectorSwizzle<DirectX::XM_SWIZZLE_Y, DirectX::XM_SWIZZLE_Z, DirectX::XM_SWIZZLE_W, DirectX::XM_SWIZZLE_X>(swizzleVector);

			DirectX::XMUINT4 swizzledTexelColor{};
			DirectX::XMStoreUInt4(&swizzledTexelColor, swizzleVector);

			texel = aiTexel{
				.b = static_cast<std::uint8_t>(swizzledTexelColor.x),
				.g = static_cast<std::uint8_t>(swizzledTexelColor.y),
				.r = static_cast<std::uint8_t>(swizzledTexelColor.z),
				.a = static_cast<std::uint8_t>(swizzledTexelColor.w)
			};
		}

		const DirectX::Image embeddedImage{
			.width = mTexturePtr->mWidth,
			.height = mTexturePtr->mHeight,

			// Assimp's documentation states nothing about the color space in which these
			// embedded texel values lie in. We'll assume that it is in sRGB space, since
			// this is typically the color space in which assets are authored. This is 
			// "acceptable" because we warn the user that the output texture may be
			// incorrect.
			.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,

			.rowPitch = static_cast<std::size_t>(mTexturePtr->mWidth) * sizeof(aiTexel),
			.slicePitch = static_cast<std::size_t>(mTexturePtr->mWidth) * static_cast<std::size_t>(mTexturePtr->mHeight) * sizeof(aiTexel),
			.pixels = reinterpret_cast<std::uint8_t*>(swizzledTexelArr.data())
		};

		DirectX::ScratchImage scratchImage{};
		Util::General::CheckHRESULT(DirectX::Convert(
			embeddedImage,
			Brawler::GetIntermediateTextureFormat<TextureType>(),
			DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
			DirectX::TEX_THRESHOLD_DEFAULT,
			scratchImage
		));

		return scratchImage;
	}

	template <aiTextureType TextureType>
	DirectX::ScratchImage EmbeddedTextureModelTextureBuilder<TextureType>::CreateIntermediateScratchTextureFromCompressedEmbeddedTexture() const
	{
		// mTexturePtr contains the texture data, but it is not interpreted properly.
		// In this case, we will let DirectXTex handle the conversion by assuming that it
		// is in a WIC-compatible format.

		DirectX::ScratchImage wicImage{};
		Util::General::CheckHRESULT(DirectX::LoadFromWICMemory(
			mTexturePtr->pcData,
			mTexturePtr->mWidth,
			DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
			nullptr,
			wicImage
		));

		DirectX::ScratchImage scratchImage{};
		Util::General::CheckHRESULT(DirectX::Convert(
			wicImage.GetImages(),
			wicImage.GetImageCount(),
			wicImage.GetMetadata(),
			Brawler::GetIntermediateTextureFormat<TextureType>(),
			DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
			DirectX::TEX_THRESHOLD_DEFAULT,
			scratchImage
		));

		return scratchImage;
	}
}