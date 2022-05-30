module;
#include <string>
#include <filesystem>
#include <memory>
#include <assimp/material.h>
#include <DxDef.h>
#include <DirectXTex.h>

export module Brawler.ModelTexture;
import Brawler.FilePathHash;
import Util.General;
import Util.ModelTexture;
import Util.ModelExport;
import Brawler.LaunchParams;
import Brawler.LODScene;
import Brawler.TextureTypeMap;
import Brawler.PolymorphicAdapter;
import Brawler.I_ModelTextureUpdateState;
import Brawler.MipMapGenerationModelTextureUpdateState;
export import Brawler.ModelTextureUpdateStateTraits;
import Util.Win32;
import Brawler.I_ModelTextureBuilder;

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
	public:
		ModelTexture() = delete;
		explicit ModelTexture(std::unique_ptr<I_ModelTextureBuilder<TextureType>>&& textureBuilderPtr);

		ModelTexture(const ModelTexture<TextureType>& rhs) = delete;
		ModelTexture& operator=(const ModelTexture<TextureType>& rhs) = delete;

		ModelTexture(ModelTexture<TextureType>&& rhs) noexcept = default;
		ModelTexture& operator=(ModelTexture<TextureType>&& rhs) noexcept = default;

		void GenerateIntermediateScratchTexture();

		void Update();
		bool IsReadyForSerialization() const;

		void WriteToFileSystem() const;
		FilePathHash GetOutputPathHash() const;

	private:
		void InitializeOutputPathInformation();

		Brawler::D3D12_RESOURCE_DESC CreateD3D12ResourceDescription() const;

	private:
		DirectX::ScratchImage mScratchTexture;
		std::unique_ptr<I_ModelTextureBuilder<TextureType>> mTextureBuilderPtr;
		FilePathHash mOutputPathHash;
		std::filesystem::path mOutputPath;
		PolymorphicAdapter<I_ModelTextureUpdateState, TextureType> mTextureUpdateAdapter;
	};
}

// ------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
	ModelTexture<TextureType>::ModelTexture(std::unique_ptr<I_ModelTextureBuilder<TextureType>>&& textureBuilderPtr) :
		mScratchTexture(),
		mTextureBuilderPtr(std::move(textureBuilderPtr)),
		mOutputPathHash(),
		mOutputPath(),
		mTextureUpdateAdapter(MipMapGenerationModelTextureUpdateState<TextureType>{})
	{
		InitializeOutputPathInformation();
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
		// format, then the conversion to the final format is essentially a no-op.
		
		mScratchTexture = mTextureBuilderPtr->CreateIntermediateScratchTexture();
	}

	template <aiTextureType TextureType>
	void ModelTexture<TextureType>::Update()
	{
		std::optional<PolymorphicAdapter<I_ModelTextureUpdateState, TextureType>> optionalNextStateAdapter{};
		
		do
		{
			optionalNextStateAdapter.reset();

			// Access the PolymorphicAdapter to update the texture for the current state.
			mTextureUpdateAdapter.AccessData([this]<typename UpdateStateType>(UpdateStateType& updateState)
			{
				updateState.UpdateTextureScratchImage(mScratchTexture);
			});

			// Try to move to the next state. If we can, then we continue looping; otherwise,
			// we exit the loop.
			mTextureUpdateAdapter.AccessData([&optionalNextStateAdapter]<typename UpdateStateType>(const UpdateStateType& updateState)
			{
				auto optionalNextState{ updateState.GetNextState() };

				if (optionalNextState.has_value())
					optionalNextStateAdapter.emplace(std::move(*optionalNextState));
			});

			if (optionalNextStateAdapter.has_value())
				mTextureUpdateAdapter = std::move(*optionalNextStateAdapter);
		} while (optionalNextStateAdapter.has_value());
	}

	template <aiTextureType TextureType>
	bool ModelTexture<TextureType>::IsReadyForSerialization() const
	{
		return mTextureUpdateAdapter.AccessData([]<typename UpdateStateType>(const UpdateStateType& updateState)
		{
			return updateState.IsFinalTextureReadyForSerialization();
		});
	}

	/*
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
	*/

	template <aiTextureType TextureType>
	FilePathHash ModelTexture<TextureType>::GetOutputPathHash() const
	{
		return mOutputPathHash;
	}

	template <aiTextureType TextureType>
	void ModelTexture<TextureType>::InitializeOutputPathInformation()
	{
		// Let [Root Output Directory] be the file path of the root directory for outputting
		// source asset files. If the name of our mesh is [Model Name], then the output file path
		// for this texture is
		// [Root Output Directory]\Textures\[Model Name]\[Unique Texture Name].btex.
		// 
		// The resolvedTextureNameHash is appended to the original texture name in order to guarantee
		// that files do not conflict with each other upon export. For this function, the details of
		// how this FilePathHash is generated are irrelevant; all that matters is that it is a unique
		// value for each texture across all LOD meshes.

		const Brawler::LaunchParams& launchParams{ Util::ModelExport::GetLaunchParameters() };

		std::filesystem::path outputTextureSubDirectoryRelativeToRoot{ IMPL::TEXTURES_FOLDER_PATH / launchParams.GetModelName() / mTextureBuilderPtr->GetUniqueTextureName() };
		outputTextureSubDirectoryRelativeToRoot.replace_extension(IMPL::BTEX_EXTENSION_PATH);

		mOutputPathHash = FilePathHash{ outputTextureSubDirectoryRelativeToRoot.c_str() };
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