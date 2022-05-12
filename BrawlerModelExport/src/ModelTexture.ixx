module;
#include <string>
#include <filesystem>
#include <assimp/material.h>
#include <DirectXTex.h>
#include "DxDef.h"

export module Brawler.ModelTexture;
import Brawler.FilePathHash;
import Util.General;
import Util.ModelTexture;
import Util.ModelExport;
import Brawler.AppParams;
import Brawler.LODScene;
import Brawler.TextureTypeMap;

namespace Brawler
{
	namespace IMPL
	{
		static constexpr std::wstring_view TEXTURES_FOLDER_NAME{ L"Textures" };
		static const std::filesystem::path TEXTURES_FOLDER_PATH{ TEXTURES_FOLDER_NAME };
		
		static constexpr std::wstring_view BTEX_EXTENSION{ L".btex" };
		static const std::filesystem::path BTEX_EXTENSION_PATH{ BTEX_EXTENSION };
	}
}

export namespace Brawler
{
	template <aiTextureType TextureType>
	class ModelTexture : private Brawler::ModelTextureMipMapGeneratorType<TextureType>
	{
	private:
		struct ModelTextureInfo
		{
			aiString OriginalTextureName;

			/// <summary>
			/// This is a FilePathHash which uniquely identifies this ModelTexture. It is
			/// used to construct the output path for the texture file upon export.
			/// </summary>
			FilePathHash ResolvedTextureNameHash;

			LODScene Scene;
		};

	public:
		ModelTexture() = default;
		explicit ModelTexture(ModelTextureInfo&& textureInfo);

		ModelTexture(const ModelTexture<TextureType>& rhs) = delete;
		ModelTexture& operator=(const ModelTexture<TextureType>& rhs) = delete;

		ModelTexture(ModelTexture<TextureType>&& rhs) noexcept = default;
		ModelTexture& operator=(ModelTexture<TextureType>&& rhs) noexcept = default;

		void GenerateIntermediateScratchTexture();

		void WriteToFileSystem() const;

		FilePathHash GetOutputPathHash() const;

	private:
		void InitializeOutputPathInformation(const FilePathHash resolvedTextureNameHash);

		Brawler::D3D12_RESOURCE_DESC CreateD3D12ResourceDescription() const;

	private:
		DirectX::ScratchImage mScratchTexture;
		aiString mOriginalTextureName;
		LODScene mScene;
		FilePathHash mOutputPathHash;
		std::filesystem::path mOutputPath;
	};
}

// ------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
	ModelTexture<TextureType>::ModelTexture(ModelTextureInfo&& textureInfo) :
		mScratchTexture(),
		mOriginalTextureName(std::move(textureInfo.OriginalTextureName)),
		mScene(std::move(textureInfo.Scene)),
		mOutputPathHash(),
		mOutputPath()
	{
		InitializeOutputPathInformation(textureInfo.ResolvedTextureNameHash);
	}

	template <aiTextureType TextureType>
	void ModelTexture<TextureType>::GenerateIntermediateScratchTexture()
	{
		// Convert the texture to the appropriate intermediate texture format. This serves as a space where
		// we can do additional texture manipulation operations before we finally export to our desired
		// format.
		//
		// Why do we do it like this? The answer is that some formats make it more difficult to perform some
		// types of operations. For example, DirectXTex does not support automatically generating mip-maps for
		// block-compressed textures.
		//
		// Don't worry, though: If the intermediate format for a given texture type is the same as its final
		// format, then the conversion to the final format is essentially a no-op. (The relevant function
		// Util::ModelTexture::ConvertTextureToDesiredFormat() uses an if-constexpr to see if the conversion
		// process is necessary.)
		mScratchTexture = Util::ModelTexture::CreateIntermediateTexture<TextureType>(mScene, mOriginalTextureName);

		// Begin generating mip-maps.
		ModelTextureMipMapGeneratorType<TextureType>::BeginMipMapGeneration();
	}

	template <aiTextureType TextureType>
	void ModelTexture<TextureType>::WriteToFileSystem() const
	{
		// First, we need to make sure that the texture is in the proper format.
		const DirectX::ScratchImage convertedImage{ Util::ModelTexture::ConvertTextureToDesiredFormat<TextureType>(mScratchTexture) };

		// Save the texture as a DDS file. This will be loaded at runtime by the Brawler
		// Engine.
		DirectX::Blob ddsTextureBlob{};
		Util::General::CheckHRESULT(DirectX::SaveToDDSMemory(
			convertedImage.GetImages(),
			convertedImage.GetImageCount(),
			convertedImage.GetMetadata(),
			DirectX::DDS_FLAGS::DDS_FLAGS_NONE,
			ddsTextureBlob
		));

		// Save the texture file to the file system. It will then be ready for packing into
		// the BPK archive.
		Util::ModelTexture::WriteTextureToFile(Util::ModelTexture::TextureWriteInfo{
			.OutputDirectory{ mOutputPath },
			.ResourceDescription{ CreateD3D12ResourceDescription() },
			.DDSBlob{ std::move(ddsTextureBlob) }
		});
	}

	template <aiTextureType TextureType>
	FilePathHash ModelTexture<TextureType>::GetOutputPathHash() const
	{
		return mOutputPathHash;
	}

	template <aiTextureType TextureType>
	void ModelTexture<TextureType>::InitializeOutputPathInformation(const FilePathHash resolvedTextureNameHash)
	{
		// Let [Root Output Directory] be the file path of the root directory for outputting
		// source asset files. If the name of our mesh is [Model Name], then the output file path
		// for this texture is
		// [Root Output Directory]\Textures\[Model Name]\[Original Texture File Name without Extension]_[resolvedTextureNameHash].btex.
		// 
		// The resolvedTextureNameHash is appended to the original texture name in order to guarantee
		// that files do not conflict with each other upon export. For this function, the details of
		// how this FilePathHash is generated are irrelevant; all that matters is that it is a unique
		// value for each texture across all LOD meshes.

		const Brawler::AppParams& launchParams{ Util::ModelExport::GetLaunchParameters() };

		// Get the output texture file name (without the extension) by getting the stem of the path
		// from the original texture name and then appending "_[resolvedTextureNameHash]".
		const std::filesystem::path textureNamePathNoExt{ std::filesystem::path{ mOriginalTextureName.C_Str() }.stem() };
		const std::wstring uniqueTextureName{ std::format(L"{}_{}", textureNamePathNoExt.c_str(), Util::General::StringToWString(resolvedTextureNameHash.GetHashString())) };

		std::filesystem::path outputTextureSubDirectoryRelativeToRoot{ IMPL::TEXTURES_FOLDER_PATH / launchParams.GetModelName() / uniqueTextureName};
		outputTextureSubDirectoryRelativeToRoot.replace_extension(IMPL::BTEX_EXTENSION_PATH);

		mOutputPathHash = FilePathHash{ outputTextureSubDirectoryRelativeToRoot.wstring() };
		mOutputPath = std::filesystem::path{ launchParams.GetRootOutputDirectory() } / outputTextureSubDirectoryRelativeToRoot;
	}

	template <aiTextureType TextureType>
	Brawler::D3D12_RESOURCE_DESC ModelTexture<TextureType>::CreateD3D12ResourceDescription() const
	{
		Brawler::D3D12_RESOURCE_DESC resourceDesc{};
		const DirectX::TexMetadata& metadata{ mScratchTexture.GetMetadata() };

		// The values of the DirectX::TEX_DIMENSION enumeration (used for metadata.dimension)
		// are explicitly assigned to be their corresponding values in the
		// D3D12_RESOURCE_DIMENSION enumeration.
		resourceDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);

		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.Width = metadata.width;
		resourceDesc.Height = static_cast<std::uint32_t>(metadata.height);

		if (metadata.dimension != DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE3D)
			resourceDesc.DepthOrArraySize = static_cast<std::uint16_t>(metadata.arraySize);
		else
			resourceDesc.DepthOrArraySize = static_cast<std::uint16_t>(metadata.depth);

		resourceDesc.MipLevels = static_cast<std::uint16_t>(metadata.mipLevels);
		resourceDesc.Format = metadata.format;
		resourceDesc.SampleDesc = DXGI_SAMPLE_DESC{
			.Count = 1,
			.Quality = 0
		};
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

		return resourceDesc;
	}
}