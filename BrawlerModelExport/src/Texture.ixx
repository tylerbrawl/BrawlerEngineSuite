module;
#include <string>
#include <filesystem>
#include <assimp/material.h>
#include <DirectXTex.h>
#include "DxDef.h"

export module Brawler.Texture;
import Brawler.FilePathHash;
import Util.General;
import Util.Texture;
import Util.ModelExport;
import Brawler.AppParams;

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
	class Texture
	{
	public:
		Texture() = default;
		explicit Texture(const aiString& textureName);

		Texture(const Texture<TextureType>& rhs) = delete;
		Texture& operator=(const Texture<TextureType>& rhs) = delete;

		Texture(Texture<TextureType>&& rhs) noexcept = default;
		Texture& operator=(Texture<TextureType>&& rhs) noexcept = default;

		void GenerateMipMaps();
		void WriteToFileSystem() const;

		FilePathHash GetOutputPathHash() const;

	private:
		void InitializeOutputPathInformation(const aiString& textureName);

		Brawler::D3D12_RESOURCE_DESC CreateD3D12ResourceDescription() const;

	private:
		DirectX::ScratchImage mScratchTexture;
		FilePathHash mOutputPathHash;
		std::filesystem::path mOutputPath;
	};
}

// ------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
	Texture<TextureType>::Texture(const aiString& textureName) :
		mScratchTexture(Util::Texture::CreateIntermediateTexture<TextureType>(textureName)),
		mOutputPathHash(),
		mOutputPath()
	{
		InitializeOutputPathInformation(textureName);
	}

	template <aiTextureType TextureType>
	void Texture<TextureType>::GenerateMipMaps()
	{
		mScratchTexture = Util::Texture::GenerateMipMaps<TextureType>(mScratchTexture);
	}

	template <aiTextureType TextureType>
	void Texture<TextureType>::WriteToFileSystem() const
	{
		// First, we need to make sure that the texture is in the proper format.
		const DirectX::ScratchImage convertedImage{ Util::Texture::ConvertTextureToDesiredFormat<TextureType>(mScratchTexture) };

		// Save the texture as a DDS file. This will be loaded at runtime by the Brawler
		// Engine.
		DirectX::Blob ddsTextureBlob{};
		CheckHRESULT(DirectX::SaveToDDSMemory(
			convertedImage.GetImages(),
			convertedImage.GetImageCount(),
			convertedImage.GetMetadata(),
			DirectX::DDS_FLAGS::DDS_FLAGS_NONE,
			ddsTextureBlob
		));

		// Save the texture file to the file system. It will then be ready for packing into
		// the BPK archive.
		Util::Texture::WriteTextureToFile(Util::Texture::TextureWriteInfo{
			.OutputDirectory{ mOutputPath },
			.ResourceDescription{ CreateD3D12ResourceDescription() },
			.DDSBlob{ std::move(ddsTextureBlob) }
		});
	}

	template <aiTextureType TextureType>
	FilePathHash Texture<TextureType>::GetOutputPathHash() const
	{
		return mOutputPathHash;
	}

	template <aiTextureType TextureType>
	void Texture<TextureType>::InitializeOutputPathInformation(const aiString& textureName)
	{
		// Let [Root Output Directory] be the file path of the root directory for outputting
		// source asset files. If the name of our mesh is 
		// [Mesh Parent Directory]\[Mesh Name].[Mesh File Extension], then the output directory
		// for this texture is [Root Output Directory]\Textures\[Mesh Name]\[Texture Name].btex.

		const Brawler::AppParams& launchParams{ Util::ModelExport::GetLaunchParameters() };
		const std::filesystem::path meshNamePath{ std::filesystem::path{ launchParams.InputMeshFilePath }.stem() };

		std::filesystem::path outputTextureSubDirectory{ IMPL::TEXTURES_FOLDER_PATH / meshNamePath / Util::General::StringToWString(textureName.C_Str()) };
		outputTextureSubDirectory.replace_extension(IMPL::BTEX_EXTENSION_PATH);

		mOutputPathHash = FilePathHash{ outputTextureSubDirectory.wstring() };
		mOutputPath = std::filesystem::path{ launchParams.RootOutputDirectory } / outputTextureSubDirectory;
	}

	template <aiTextureType TextureType>
	Brawler::D3D12_RESOURCE_DESC Texture<TextureType>::CreateD3D12ResourceDescription() const
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