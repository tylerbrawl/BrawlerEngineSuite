module;
#include <optional>
#include <filesystem>
#include <ranges>
#include <cassert>
#include <vector>
#include <format>
#include <span>
#include <array>
#include <assimp/scene.h>

#define NOMINMAX
#include <DirectXTex.h>
#undef NOMINMAX

export module Brawler.AssimpTextureConverter;
import :AssimpTextureMaterialColorMap;
import Brawler.ImportedMesh;
import Brawler.OptionalRef;
import Brawler.TextureTypeMap;
import Util.General;
import Util.Win32;

export namespace Brawler
{
	struct ConvertedAssimpTexture
	{
		DirectX::ScratchImage ScratchImage;
		std::optional<std::wstring> TextureName;
	};
	
	template <aiTextureType TextureType>
	class AssimpTextureConverter
	{
	public:
		AssimpTextureConverter() = default;

		AssimpTextureConverter(const AssimpTextureConverter& rhs) = delete;
		AssimpTextureConverter& operator=(const AssimpTextureConverter& rhs) = delete;

		AssimpTextureConverter(AssimpTextureConverter&& rhs) noexcept = default;
		AssimpTextureConverter& operator=(AssimpTextureConverter&& rhs) noexcept = default;

		void BeginTextureConversion(const ImportedMesh& mesh);

		std::size_t GetConvertedTextureCount() const;
		bool HasConvertedTextures() const;

		std::span<ConvertedAssimpTexture> GetConvertedTextureSpan();
		std::span<const ConvertedAssimpTexture> GetConvertedTextureSpan() const;

		bool ReferencesSameTextures(const AssimpTextureConverter& otherConverter) const;

	private:
		void BeginTextureConversion(const ImportedMesh& mesh, const std::uint32_t textureIndex);

		static std::optional<ConvertedAssimpTexture> TryCreateTextureFromMaterialColor(const ImportedMesh& mesh);
		static ConvertedAssimpTexture CreateTextureFromExternalFile(const ImportedMesh& mesh, const aiString& filePath);
		static ConvertedAssimpTexture CreateTextureFromEmbeddedCompressedFile(const ImportedMesh& mesh, const aiString& textureName, const aiTexture& embeddedTexture);
		static ConvertedAssimpTexture CreateTextureFromEmbeddedUncompressedData(const ImportedMesh& mesh, const aiString& textureName, const aiTexture& embeddedTexture);

	private:
		std::vector<ConvertedAssimpTexture> mConvertedTextureArr;
	};
}

namespace Brawler
{
	template <aiTextureType TextureType>
	void AssimpTextureConverter<TextureType>::BeginTextureConversion(const ImportedMesh& mesh)
	{
		const std::uint32_t numTexturesOfRelevantType = mesh.GetMeshMaterial().GetTextureCount(TextureType);

		if (numTexturesOfRelevantType > 0) [[likely]]
		{
			mConvertedTextureArr.resize(numTexturesOfRelevantType);
			
			for (auto i : std::views::iota(0u, numTexturesOfRelevantType))
				BeginTextureConversion(mesh, i);
		}
		else
		{
			// If we could not find any textures of the corresponding type within the ImportedMesh's
			// aiMaterial, but TextureType has an associated AssimpMaterialKeyID, then we try to create
			// a texture from the material color. For instance, aiTextureType_DIFFUSE corresponds to
			// AssimpMaterialKeyID::COLOR_DIFFUSE, so that is what we would search for in that case.

			constexpr std::optional<AssimpMaterialKeyID> MATERIAL_KEY_ID{ GetMaterialColor<TextureType>() };

			if constexpr (MATERIAL_KEY_ID.has_value())
			{
				std::optional<ConvertedAssimpTexture> convertedMaterialColorTexture{ TryCreateTextureFromMaterialColor(mesh) };

				if (convertedMaterialColorTexture.has_value()) [[likely]]
					mConvertedTextureArr.push_back(std::move(*convertedMaterialColorTexture));
			}
		}
	}

	template <aiTextureType TextureType>
	std::size_t AssimpTextureConverter<TextureType>::GetConvertedTextureCount() const
	{
		return mConvertedTextureArr.size();
	}

	template <aiTextureType TextureType>
	bool AssimpTextureConverter<TextureType>::HasConvertedTextures() const
	{
		return !mConvertedTextureArr.empty();
	}

	template <aiTextureType TextureType>
	std::span<ConvertedAssimpTexture> AssimpTextureConverter<TextureType>::GetConvertedTextureSpan()
	{
		return std::span<ConvertedAssimpTexture>{ mConvertedTextureArr };
	}

	template <aiTextureType TextureType>
	std::span<const ConvertedAssimpTexture> AssimpTextureConverter<TextureType>::GetConvertedTextureSpan() const
	{
		return std::span<const ConvertedAssimpTexture>{ mConvertedTextureArr };
	}

	template <aiTextureType TextureType>
	bool AssimpTextureConverter<TextureType>::ReferencesSameTextures(const AssimpTextureConverter<TextureType>& otherConverter) const
	{
		if (mConvertedTextureArr.size() != otherConverter.mConvertedTextureArr.size())
			return false;

		const std::span<const ConvertedAssimpTexture> thisTextureSpan{ GetConvertedTextureSpan() };
		const std::span<const ConvertedAssimpTexture> otherTextureSpan{ otherConverter.GetConvertedTextureSpan() };

		// Still no std::views::zip...
		for (auto i : std::views::iota(0u, thisTextureSpan.size()))
		{
			const ConvertedAssimpTexture& currThisTexture{ thisTextureSpan[i] };
			const ConvertedAssimpTexture& currOtherTexture{ otherTextureSpan[i] };

			// The TextureName field of the ConvertedAssimpTexture struct can be used to detect duplicate
			// textures. They are generated such that if two TextureNames are equivalent, then it is guaranteed
			// that they refer to the same texture. However, the opposite is not necessarily true.
			//
			// Specifically, textures created from material colors will never have an associated texture name.
			// Likewise, embedded textures from different LOD meshes will never have the same texture name.
			if (!currThisTexture.TextureName.has_value() || !currOtherTexture.TextureName.has_value() || *(currThisTexture.TextureName) != *(currOtherTexture.TextureName))
				return false;
		}

		return true;
	}

	template <aiTextureType TextureType>
	void AssimpTextureConverter<TextureType>::BeginTextureConversion(const ImportedMesh& mesh, const std::uint32_t textureIndex)
	{
		assert(textureIndex < mesh.GetMeshMaterial().GetTextureCount(TextureType) && "ERROR: An out-of-bounds textureIndex was specified in a call to AssimpTextureConverter::BeginTextureConversion()! (The relevant texture count can be determined by calling aiMaterial::GetTextureCount().)");
		assert(textureIndex < mConvertedTextureArr.size());

		aiString texturePath{};

		{
			const aiReturn getTextureReturn = mesh.GetMeshMaterial().GetTexture(TextureType, textureIndex, std::addressof(texturePath));
			assert(getTextureReturn == aiReturn::aiReturn_SUCCESS);
		}

		const aiTexture* embeddedTexturePtr = mesh.GetOwningScene().GetEmbeddedTexture(texturePath.C_Str());

		if (embeddedTexturePtr == nullptr)
			mConvertedTextureArr[textureIndex] = CreateTextureFromExternalFile(mesh, texturePath);

		else if (embeddedTexturePtr->mHeight == 0)
			mConvertedTextureArr[textureIndex] = CreateTextureFromEmbeddedCompressedFile(mesh, texturePath, *embeddedTexturePtr);

		else
			mConvertedTextureArr[textureIndex] = CreateTextureFromEmbeddedUncompressedData(mesh, texturePath, *embeddedTexturePtr);
	}

	template <aiTextureType TextureType>
	std::optional<ConvertedAssimpTexture> AssimpTextureConverter<TextureType>::TryCreateTextureFromMaterialColor(const ImportedMesh& mesh)
	{
		// Assimp stores its material colors as floats in the range [0.0f, 1.0f]. We will convert 
		// these to the R8G8B8A8_UNORM_SRGB format. We choose the _SRGB format with the assumption 
		// that the color is specified as an sRGB value, since this is the color space in which assets 
		// are usually authored in.
		
		constexpr AssimpMaterialKeyID MATERIAL_KEY_ID{ *(GetMaterialColor<TextureType>()) };
		const std::optional<aiColor3D> materialColor{ Brawler::GetAssimpMaterialProperty<MATERIAL_KEY_ID>(mesh.GetMeshMaterial()) };

		if (!materialColor.has_value())
			return std::optional<ConvertedAssimpTexture>{};

		std::array<std::uint8_t, 4> rgbaColorArr{
			static_cast<std::uint8_t>(materialColor->r * 255.0f),
			static_cast<std::uint8_t>(materialColor->g * 255.0f),
			static_cast<std::uint8_t>(materialColor->b * 255.0f),
			std::numeric_limits<std::uint8_t>::max()
		};

		DirectX::ScratchImage convertedImage{};

		if constexpr (Brawler::GetIntermediateTextureFormat<TextureType>() != DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
		{
			const DirectX::Image srcImage{
				.width = 1,
				.height = 1,
				.format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				.rowPitch = sizeof(rgbaColorArr),
				.slicePitch = sizeof(rgbaColorArr),
				.pixels = rgbaColorArr.data()
			};

			Util::General::CheckHRESULT(DirectX::Convert(
				srcImage,
				Brawler::GetIntermediateTextureFormat<TextureType>(),
				DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
				DirectX::TEX_THRESHOLD_DEFAULT,
				convertedImage
			));
		}
		else
		{
			Util::General::CheckHRESULT(convertedImage.Initialize2D(
				Brawler::GetIntermediateTextureFormat<TextureType>(),
				1,
				1,
				1,
				1,
				DirectX::CP_FLAGS::CP_FLAGS_NONE
			));

			const DirectX::Image* const relevantImagePtr = convertedImage.GetImage(0, 0, 0);
			assert(relevantImagePtr != nullptr);

			std::memcpy(relevantImagePtr->pixels, rgbaColorArr.data(), sizeof(rgbaColorArr));
		}

		return ConvertedAssimpTexture{
			.ScratchImage{std::move(convertedImage)},
			.TextureName{}
		};
	}

	template <aiTextureType TextureType>
	ConvertedAssimpTexture AssimpTextureConverter<TextureType>::CreateTextureFromExternalFile(const ImportedMesh& mesh, const aiString& filePath)
	{
		std::filesystem::path textureFilePath{ filePath.C_Str() };

		std::error_code errorCode{};
		bool fileExists = std::filesystem::exists(textureFilePath, errorCode);

		if (errorCode) [[unlikely]]
			throw std::runtime_error{ std::format(R"(ERROR: The attempt to check if the file at "{}" exists failed with the following error: {})", textureFilePath.string(), errorCode.message()) };

		if (!fileExists)
		{
			// If we can't find the file like that, then perhaps it was specified relative to the LOD mesh file directory.
			textureFilePath = mesh.GetLODScene().GetInputMeshFilePath().parent_path() / textureFilePath;

			fileExists = std::filesystem::exists(textureFilePath, errorCode);

			if (errorCode) [[unlikely]]
				throw std::runtime_error{ std::format(R"(ERROR: The attempt to check if the file at "{}" exists failed with the following error: {})", textureFilePath.string(), errorCode.message()) };

			if (!fileExists) [[unlikely]]
				throw std::runtime_error{ std::format(R"(ERROR: The external texture file "{}" referenced by the LOD mesh at "{}" could not be found!)", filePath.C_Str(), mesh.GetLODScene().GetInputMeshFilePath().string()) };
		}

		const bool isDirectory = std::filesystem::is_directory(textureFilePath, errorCode);

		if (errorCode) [[unlikely]]
			throw std::runtime_error{ std::format(R"(ERROR: The attempt to check if the file at "{}" was actually a directory failed with the following error: {})", textureFilePath.string(), errorCode.message()) };

		if (isDirectory) [[unlikely]]
			throw std::runtime_error{ std::format(R"(ERROR: The external texture file "{}" referenced by the LOD mesh at "{}" is actually a directory!)", filePath.C_Str(), mesh.GetLODScene().GetInputMeshFilePath().string()) };

		// Transform the texture file path into its canonical path. That way, we can compare it to other textures referring
		// to the same file on the system, but through different means (e.g., as a symbolic link).
		textureFilePath = std::filesystem::canonical(textureFilePath, errorCode);

		if (errorCode) [[unlikely]]
			throw std::runtime_error{ std::format(R"(ERROR: The attempt to get the canonical file path of "{}" failed with the following error: {})", textureFilePath.string(), errorCode.message()) };

		DirectX::ScratchImage scratchImage{};
		Util::General::CheckHRESULT(DirectX::LoadFromWICFile(
			textureFilePath.c_str(),
			DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
			nullptr,
			scratchImage
		));

		DirectX::ScratchImage intermediateScratchImage{};
		Util::General::CheckHRESULT(DirectX::Convert(
			scratchImage.GetImages(),
			scratchImage.GetImageCount(),
			scratchImage.GetMetadata(),
			Brawler::GetIntermediateTextureFormat<TextureType>(),
			DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
			DirectX::TEX_THRESHOLD_DEFAULT,
			intermediateScratchImage
		));

		return ConvertedAssimpTexture{
			.ScratchImage{ std::move(intermediateScratchImage) },
			.TextureName{ textureFilePath.wstring() }
		};
	}

	template <aiTextureType TextureType>
	ConvertedAssimpTexture AssimpTextureConverter<TextureType>::CreateTextureFromEmbeddedCompressedFile(const ImportedMesh& mesh, const aiString& textureName, const aiTexture& embeddedTexture)
	{
		DirectX::ScratchImage wicScratchImage{};
		Util::General::CheckHRESULT(DirectX::LoadFromWICMemory(
			embeddedTexture.pcData,
			embeddedTexture.mWidth,
			DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
			nullptr,
			wicScratchImage
		));

		DirectX::ScratchImage intermediateScratchImage{};
		Util::General::CheckHRESULT(DirectX::Convert(
			wicScratchImage.GetImages(),
			wicScratchImage.GetImageCount(),
			wicScratchImage.GetMetadata(),
			Brawler::GetIntermediateTextureFormat<TextureType>(),
			DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
			DirectX::TEX_THRESHOLD_DEFAULT,
			intermediateScratchImage
		));

		return ConvertedAssimpTexture{
			.ScratchImage{ std::move(intermediateScratchImage) },
			.TextureName{ std::format(L"LOD{}_{}", mesh.GetLODScene().GetLODLevel(), Util::General::StringToWString(textureName.C_Str())) }
		};
	}

	template <aiTextureType TextureType>
	ConvertedAssimpTexture AssimpTextureConverter<TextureType>::CreateTextureFromEmbeddedUncompressedData(const ImportedMesh& mesh, const aiString& textureName, const aiTexture& embeddedTexture)
	{
		// mTexturePtr contains the actual texture data. So, we fill a DirectX::Image 
		// structure with the appropriate data and create the scratch texture with that.
		//
		// Unfortunately, A8R8G8B8_UNORM format is not a valid DXGI_FORMAT enumeration, so
		// we have to do some swizzling. To prevent potential errors, we need to create a copy 
		// of the texture data and swizzle that, instead.

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

		const DirectX::Image embeddedImage{
			.width = embeddedTexture.mWidth,
			.height = embeddedTexture.mHeight,

			// Assimp's documentation states nothing about the color space in which these
			// embedded texel values lie in. We'll assume that it is in sRGB space, since
			// this is typically the color space in which assets are authored. This is 
			// "acceptable" because we warn the user that the output texture may be
			// incorrect.
			.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,

			.rowPitch = static_cast<std::size_t>(embeddedTexture.mWidth) * sizeof(aiTexel),
			.slicePitch = static_cast<std::size_t>(embeddedTexture.mWidth) * static_cast<std::size_t>(embeddedTexture.mHeight) * sizeof(aiTexel),
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

		return ConvertedAssimpTexture{
			.ScratchImage{ std::move(scratchImage) },
			.TextureName{ std::format(L"LOD{}_{}", mesh.GetLODScene().GetLODLevel(), Util::General::StringToWString(textureName.C_Str())) }
		};
	}
}