module;
#include <span>
#include <filesystem>
#include <assimp/scene.h>
#include <DirectXTex.h>
#include "DxDef.h"

export module Util.ModelTexture;
import Brawler.TextureTypeMap;
import Brawler.LaunchParams;
import Util.ModelExport;
import Util.Win32;
import Brawler.Win32.ConsoleFormat;
import Util.General;
import Brawler.FilePathHash;
import Brawler.LODScene;

export namespace Util
{
	namespace ModelTexture
	{
		constexpr std::string_view EMBEDDED_TEXTURE_NAME_FORMAT_STR = "EmbeddedLOD{}_{}";

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
		/// <param name="lodScene">
		/// - The LODScene instance referring to the aiScene which contains the relevant
		///   texture.
		/// </param>
		/// <param name="textureName">
		/// - The name of the texture which will be converted.
		/// </param>
		/// <returns>
		/// The function returns a DirectX::ScratchImage containing the intermediate texture.
		/// This can be used in future texture manipulation operations.
		/// </returns>
		template <aiTextureType TextureType>
		DirectX::ScratchImage CreateIntermediateTexture(const Brawler::LODScene& lodScene, const aiString& textureName);

		/// <summary>
		/// Writes the texture data specified by writeInfo to the filesystem. See the documentation
		/// of Util::ModelTexture::TextureWriteInfo for more information.
		/// </summary>
		/// <param name="writeInfo">
		/// - A TextureWriteInfo instance which includes all of the necessary information for
		///   writing out the texture file.
		/// </param>
		void WriteTextureToFile(const TextureWriteInfo& writeInfo);
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
			Util::Win32::WriteFormattedConsoleMessage(std::string{ "WARNING: The embedded texture " } + embeddedTexture.mFilename.C_Str() + " was interpreted by Assimp ambiguously. The resulting texture file, if produced, may be incorrect for several reasons, such as color space inconsistency. Please use either unembedded textures or embedded textures in a compressed format, such as .png.", Brawler::Win32::ConsoleFormat::WARNING);

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
		DirectX::ScratchImage CreateIntermediateTextureFromFile(const Brawler::LODScene& scene, const aiString& textureName)
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

				const std::filesystem::path meshFilePath{ Util::ModelExport::GetLaunchParameters().GetLODFilePath(scene.GetLODLevel()) };
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
		DirectX::ScratchImage CreateIntermediateTexture(const Brawler::LODScene& scene, const aiString& textureName)
		{
			// First, check to see if the texture is embedded.
			const aiTexture* embeddedTexture{ scene.GetScene().GetEmbeddedTexture(textureName.C_Str())};
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
			
			return CreateIntermediateTextureFromFile<TextureType>(scene, textureName);
		}
	}
}