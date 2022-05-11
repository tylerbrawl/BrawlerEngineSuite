module;
#include <span>
#include <filesystem>
#include <assimp/scene.h>
#include <DirectXTex.h>
#include "DxDef.h"

export module Util.ModelTexture;
import Brawler.TextureTypeMap;
import Brawler.AppParams;
import Util.ModelExport;
import Util.Win32;
import Util.General;
import Brawler.FilePathHash;

export namespace Util
{
	namespace ModelTexture
	{
		struct TextureWriteInfo
		{
			/// <summary>
			/// The full file path, including the extension, of the .btex file which is to be
			/// written.
			/// </summary>
			std::filesystem::path OutputDirectory;

			/// <summary>
			/// The DirectX12 resource description which describes this texture.
			/// </summary>
			Brawler::D3D12_RESOURCE_DESC ResourceDescription;

			/// <summary>
			/// A DirectX::Blob instance containing the texture data saved as a DDS file. This
			/// can/should be created with DirectX::SaveToDDSMemory().
			/// </summary>
			DirectX::Blob DDSBlob;
		};

		/// <summary>
		/// Creates an intermediate DDS texture by finding the texture specified by textureName
		/// and converting it using DirectXTex. The function will search either the aiScene or
		/// the filesystem as needed in order to find the specified texture.
		/// 
		/// This intermediate texture can be used by later DirectXTex operations to modify it
		/// before exporting.
		/// </summary>
		/// <param name="textureName">
		/// - The name of the texture which will be converted.
		/// </param>
		/// <returns>
		/// The function returns a DirectX::ScratchImage containing the intermediate texture.
		/// This can be used in future texture manipulation operations.
		/// </returns>
		template <aiTextureType TextureType>
		DirectX::ScratchImage CreateIntermediateTexture(const aiString& textureName);

		/// <summary>
		/// Generates a mip-map chain for the provided DirectX::ScratchImage. Call this function
		/// after a texture has been converted to DDS format by calling Util::ModelTexture::CreateIntermediateTexture().
		/// 
		/// The default behavior is to use DirectXTex to automatically create a mip-map chain by
		/// repeatedly downsampling the image. This works fine for things likes diffuse albedo
		/// textures, but it can cause problems for other texture types, such as normal maps. For these,
		/// one should create an explicit template specialization of this function.
		/// </summary>
		/// <param name="texture">
		/// - The texture for which a mip-map chain is to be generated.
		/// </param>
		/// <returns>
		/// The function returns a DirectX::ScratchImage which contains a mip-map chain generated
		/// from the input texture.
		/// </returns>
		template <aiTextureType TextureType>
		DirectX::ScratchImage GenerateMipMaps(const DirectX::ScratchImage& texture);

		/// <summary>
		/// Converts the specified texture into its final format, as specified by
		/// Brawler::IMPL::TextureTypeMap<TextureType>::DESIRED_FORMAT (see TextureTypeMap.ixx). 
		/// The returned DirectX::ScratchImage can then be written to the filesystem.
		/// 
		/// This function should only be called after all texture manipulation operations
		/// have been finished.
		/// </summary>
		/// <param name="texture">
		/// - The texture which is to be converted to its corresponding final format, as
		///   specified by the TextureType template parameter.
		/// </param>
		/// <returns>
		/// The function returns a DirectX::ScratchImage in the format specified by
		/// Brawler::IMPL::TextureTypeMap<TextureType>::DESIRED_FORMAT.
		/// </returns>
		template <aiTextureType TextureType>
		DirectX::ScratchImage ConvertTextureToDesiredFormat(const DirectX::ScratchImage& texture);

		/// <summary>
		/// Writes the texture data specified by writeInfo to the filesystem. See the documentation
		/// of Util::ModelTexture::TextureWriteInfo for more information.
		/// </summary>
		/// <param name="writeInfo">
		/// - A TextureWriteInfo instance which includes all of the necessary information for
		///   writing out the texture file.
		/// </param>
		void WriteTextureToFile(const TextureWriteInfo& writeInfo);

		Brawler::FilePathHash GetTextureFilePathHash(const aiString& textureName);
	}
}

// --------------------------------------------------------------------------------------------

namespace Util
{
	namespace ModelTexture
	{
		template <aiTextureType TextureType>
		DirectX::ScratchImage CreateIntermediateTextureFromAssimpInterpretedEmbeddedTexture(const aiTexture& embeddedTexture)
		{
			// embeddedTexture contains the actual texture data. So, we fill a DirectX::Image 
			// structure with the appropriate data and create ddsImage with that.
			//
			// Unfortunately, A8R8G8B8_UNORM format is not a valid DXGI_FORMAT enumeration, so
			// we have to do some swizzling. Even more unfortunate is the fact that 
			// aiScene::GetEmbeddedTexture() returns a const aiTexture*; so, to prevent potential
			// errors, we need to create a copy of the texture data and swizzle that, instead.

			// Warn the user that this texture may have been processed incorrectly.
			Util::Win32::WriteFormattedConsoleMessage(std::string{ "WARNING: The embedded texture " } + embeddedTexture.mFilename.C_Str() + " was interpreted by Assimp ambiguously. The resulting texture file, if produced, may be incorrect for several reasons, such as color space inconsistency. Please use either unembedded textures or embedded textures in a compressed format, such as .png.", Util::Win32::ConsoleFormat::WARNING);

			assert(std::strcmp(embeddedTexture.achFormatHint, "argb8888") == 0 && "ERROR: Assimp's documentation was lying about its embedded texture format!");

			// Create a copy of the texture data for swizzling.
			const std::size_t textureSize = static_cast<std::size_t>(embeddedTexture.mWidth) * static_cast<std::size_t>(embeddedTexture.mHeight);
			std::vector<aiTexel> swizzledTexelArr{};
			swizzledTexelArr.resize(textureSize);

			std::memcpy(swizzledTexelArr.data(), embeddedTexture.pcData, textureSize * sizeof(aiTexel));

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

			DirectX::Image embeddedImage{
				.width = embeddedTexture.mWidth,
				.height = embeddedTexture.mHeight,

				// Assimp's documentation states nothing about the color space in which these
				// embedded texel values lie in. We'll assume that it is in linear space. This
				// is "acceptable" because we warn the user that the output texture may be
				// incorrect.
				.format = DXGI_FORMAT_R8G8B8A8_UNORM,

				.rowPitch = static_cast<std::size_t>(embeddedTexture.mWidth) * sizeof(aiTexel),
				.slicePitch = static_cast<std::size_t>(embeddedTexture.mWidth) * static_cast<std::size_t>(embeddedTexture.mHeight) * sizeof(aiTexel),
				.pixels = reinterpret_cast<std::uint8_t*>(swizzledTexelArr.data())
			};

			DirectX::ScratchImage ddsImage{};
			Util::General::CheckHRESULT(DirectX::Convert(
				embeddedImage,
				Brawler::GetIntermediateTextureFormat<TextureType>(),
				DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
				DirectX::TEX_THRESHOLD_DEFAULT,
				ddsImage
			));

			return ddsImage;
		}

		template <aiTextureType TextureType>
		DirectX::ScratchImage CreateIntermediateTextureFromCompressedEmbeddedTexture(const aiTexture& embeddedTexture)
		{
			// embeddedTexture contains the texture data, but it is not interpreted properly.
			// In this case, we will let DirectXTex handle the conversion by assuming that it
			// is in a WIC-compatible format.

			DirectX::ScratchImage wicImage{};
			Util::General::CheckHRESULT(DirectX::LoadFromWICMemory(
				embeddedTexture.pcData,
				embeddedTexture.mWidth,
				DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
				nullptr,
				wicImage
			));

			DirectX::ScratchImage ddsImage{};
			Util::General::CheckHRESULT(DirectX::Convert(
				wicImage.GetImages(),
				wicImage.GetImageCount(),
				wicImage.GetMetadata(),
				Brawler::GetIntermediateTextureFormat<TextureType>(),
				DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
				DirectX::TEX_THRESHOLD_DEFAULT,
				ddsImage
			));

			return ddsImage;
		}

		template <aiTextureType TextureType>
		DirectX::ScratchImage CreateIntermediateTextureFromFile(const aiString& textureName)
		{
			// The texture is *NOT* embedded within the model. In that case, we need to read it
			// and save it to our appropriate format.

			// First, let's assume that the path is absolute.
			std::filesystem::path texturePath{ textureName.C_Str() };

			std::error_code errorCode{};
			const bool texturePathIsAbsolute = std::filesystem::exists(texturePath, errorCode);

			if (errorCode) [[unlikely]]
				throw std::runtime_error{ "ERROR: std::filesystem::exists failed to check if a texture path was absolute! The error code is as follows: " + errorCode.message() };

			if (!texturePathIsAbsolute)
			{
				// If the texture path is *NOT* absolute, then it is probably defined relative to
				// the original mesh file's directory.

				const std::filesystem::path meshFilePath{ Util::ModelExport::GetLaunchParameters().InputMeshFilePath };
				texturePath = (meshFilePath.parent_path() / texturePath);

				const bool textureFound = std::filesystem::exists(texturePath, errorCode);

				if (errorCode) [[unlikely]]
					throw std::runtime_error{ "ERROR: std::filesystem::exists failed to check if a texture path concatenated to the input mesh file's containing directory exists! The error code is as follows: " + errorCode.message() };

				if (!textureFound) [[unlikely]]
					throw std::runtime_error{ std::string{ "ERROR: The texture " } + textureName.C_Str() + " could not be found!" };
			}

			// Let DirectXTex load the file, assuming that it is in a WIC-compatible format.
			const std::wstring texturePathStr{ texturePath.wstring() };
			DirectX::ScratchImage wicImage{};

			Util::General::CheckHRESULT(DirectX::LoadFromWICFile(
				texturePathStr.c_str(),
				DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
				nullptr,
				wicImage
			));

			DirectX::ScratchImage ddsImage{};
			Util::General::CheckHRESULT(DirectX::Convert(
				wicImage.GetImages(),
				wicImage.GetImageCount(),
				wicImage.GetMetadata(),
				Brawler::GetIntermediateTextureFormat<TextureType>(),
				DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
				DirectX::TEX_THRESHOLD_DEFAULT,
				ddsImage
			));

			return ddsImage;
		}

		template <aiTextureType TextureType>
		DirectX::ScratchImage CreateIntermediateTexture(const aiString& textureName)
		{
			// First, check to see if the texture is embedded.
			const aiTexture* embeddedTexture{ Util::ModelExport::GetScene().GetEmbeddedTexture(textureName.C_Str()) };
			DirectX::ScratchImage ddsImage{};

			// Most textures, however, are likely to not be embedded.
			if (embeddedTexture != nullptr) [[unlikely]]
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

				if (embeddedTexture->mHeight != 0)
					return CreateIntermediateTextureFromAssimpInterpretedEmbeddedTexture<TextureType>(*embeddedTexture);
				
				return CreateIntermediateTextureFromCompressedEmbeddedTexture<TextureType>(*embeddedTexture);
			}
			
			return CreateIntermediateTextureFromFile<TextureType>(textureName);
		}

		template <aiTextureType TextureType>
		DirectX::ScratchImage GenerateMipMaps(const DirectX::ScratchImage& texture)
		{
			// DirectXTex does not support generating mip-map chains directly from BC formats.
			// We could decompress then, create the chain, and then compress that, but this can
			// be incredibly slow.

#ifdef _DEBUG
			const std::span<const DirectX::Image> imageSpan{ texture.GetImages(), texture.GetImageCount() };

			for (const auto& image : imageSpan)
				assert(!DirectX::IsCompressed(image.format) && "ERROR: A block-compressed image was provided for Util::ModelTexture::GenerateMipMaps()! (Did you forget to use Util::ModelTexture::CreateIntermediateTexture()?)");
#endif // _DEBUG

			DirectX::ScratchImage mipMapChain{};
			Util::General::CheckHRESULT(DirectX::GenerateMipMaps(
				texture.GetImages(),
				texture.GetImageCount(),
				texture.GetMetadata(),
				DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
				0,  // Create a full mip-map chain, down to a 1x1 texture.
				mipMapChain
			));

			return mipMapChain;
		}

		template <aiTextureType TextureType>
		DirectX::ScratchImage ConvertTextureToDesiredFormat(const DirectX::ScratchImage& texture)
		{
			// Don't do any conversions if the intermediate and desired formats are the same.
			if constexpr (Brawler::GetIntermediateTextureFormat<TextureType>() == Brawler::GetDesiredTextureFormat<TextureType>())
				return texture;
			
			DirectX::ScratchImage finalImage{};
			
			if constexpr (Brawler::IsBlockCompressedFormat<Brawler::GetDesiredTextureFormat<TextureType>()>())
			{
				// The desired texture type is in a block-compressed format, so we need to use a
				// specific DirectXTex function for that.

				Util::General::CheckHRESULT(DirectX::Compress(
					texture.GetImages(),
					texture.GetImageCount(),
					texture.GetMetadata(),
					Brawler::GetDesiredTextureFormat<TextureType>(),

					// There is a multi-threading flag provided for compression. However, since we
					// are using our own CPU job system, creating additional threads for this would
					// only introduce more context switching. So, we do not use that flag.
					DirectX::TEX_COMPRESS_FLAGS::TEX_COMPRESS_DEFAULT,

					DirectX::TEX_THRESHOLD_DEFAULT,
					finalImage
				));
			}
			else
			{
				Util::General::CheckHRESULT(DirectX::Convert(
					texture.GetImages(),
					texture.GetImageCount(),
					texture.GetMetadata(),
					Brawler::GetDesiredTextureFormat<TextureType>(),
					DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
					DirectX::TEX_THRESHOLD_DEFAULT,
					finalImage
				));
			}

			return finalImage;
		}
	}
}