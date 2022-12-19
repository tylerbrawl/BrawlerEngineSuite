module;
#include <span>
#include <vector>
#include <unordered_set>
#include <filesystem>
#include <memory>
#include <cassert>
#include <ranges>
#include <algorithm>
#include <DxDef.h>
#include <DirectXTex.h>
#include <assimp/scene.h>
#include <assimp/material.h>

module Brawler.AssimpSceneLoader;
import Brawler.FilePathHash;
import Brawler.JobSystem;
import Brawler.GenericPreFrameTextureUpdate;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.SceneTextures;
import Util.General;
import Util.Math;
import Brawler.Application;
import Brawler.D3D12.Renderer;
import Brawler.GPUSceneUpdateRenderModule;
import Brawler.SceneTextureDatabase;

namespace
{
	struct LoadedSceneTextureInfo
	{
		Brawler::SceneTexture2D SceneTexture;
		std::vector<Brawler::GenericPreFrameTextureUpdate> SceneTextureUpdateArr;
	};

	LoadedSceneTextureInfo LoadSceneTexture(const std::filesystem::path& textureFilePath)
	{
		// Use DirectXTex to load the image. Realistically, we would have some form of custom
		// file format and asynchronously stream in texture data as needed. At this point,
		// however, I just want to draw something, damnit.
		DirectX::ScratchImage intermediateScratchImage{};

		Util::General::CheckHRESULT(DirectX::LoadFromWICFile(
			textureFilePath.c_str(),
			DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
			nullptr,
			intermediateScratchImage
		));

		// TODO: This probably isn't correct for normal maps.
		DirectX::ScratchImage finalScratchImage{};
		Util::General::CheckHRESULT(DirectX::GenerateMipMaps(
			*(intermediateScratchImage.GetImage(0, 0, 0)),
			DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
			0,
			finalScratchImage
		));

		Brawler::D3D12::Texture2DBuilder textureBuilder{};
		const DirectX::TexMetadata& textureMetadata{ finalScratchImage.GetMetadata() };

		const std::size_t numMipLevels = textureMetadata.mipLevels;

		// We are expecting only 2D textures.
		assert(textureMetadata.dimension == DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE2D && "ERROR: An attempt was made to create a StandardMaterialDefinition using a texture which wasn't 2D!");

		textureBuilder.SetTextureDimensions(textureMetadata.width, textureMetadata.height);
		textureBuilder.SetMipLevelCount(numMipLevels);
		textureBuilder.SetTextureFormat(textureMetadata.format);

		textureBuilder.DenyUnorderedAccessViews();
		textureBuilder.DenySimultaneousAccess();

		textureBuilder.SetInitialResourceState(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

		std::unique_ptr<Brawler::D3D12::Texture2D> createdTexturePtr{ std::make_unique<Brawler::D3D12::Texture2D>(textureBuilder) };

		std::vector<Brawler::GenericPreFrameTextureUpdate> textureUpdateArr{};
		textureUpdateArr.reserve(numMipLevels);

		for (const auto i : std::views::iota(0ull, textureMetadata.mipLevels))
		{
			// Copy the texture data into a format suitable for texture uploads. Then, move this
			// into a GenericPreFrameTextureUpdate instance. Ideally, our texture data would be
			// in a file format which is already prepared for texture uploads.
			Brawler::D3D12::Texture2DSubResource destSubResource{ createdTexturePtr->GetSubResource(i) };
			const DirectX::Image& currMipLevelImage{ finalScratchImage.GetImage(i, 0, 0) };

			Brawler::GenericPreFrameTextureUpdate currMipLevelUpdate{ destSubResource };
			
			// If DirectXTex already aligned the data properly for D3D12 uploads, then we just
			// directly copy the data into the GenericPreFrameTextureUpdate instance. Otherwise,
			// we need to align it manually.
			const std::span<const std::uint8_t> srcTexelDataSpan{ currMipLevelImage.pixels, currMipLevelImage.slicePitch };

			if (currMipLevelImage.rowPitch % D3D12_TEXTURE_DATA_PITCH_ALIGNMENT == 0)
				currMipLevelImage.SetInputData(srcTexelDataSpan);
			else
			{
				const std::size_t alignedRowPitch = Util::Math::AlignToPowerOfTwo(currMipLevelImage.rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
				const std::size_t totalUploadDataSize = (alignedRowPitch * currMipLevelImage.height);

				std::vector<std::uint8_t> alignedMipLevelByteArr{};
				alignedMipLevelByteArr.resize(totalUploadDataSize);

				const auto destChunkedDataView{ alignedMipLevelByteArr | std::views::chunk(alignedRowPitch) };
				const auto srcChunkedDataView{ srcTexelDataSpan | std::views::chunk(currMipLevelImage.rowPitch) };

				// YES! IT'S FINALLY HERE! ALL HAIL std::views::zip!
				for (auto& [destRowData, srcRowData] : std::views::zip(destChunkedDataView, srcChunkedDataView))
					std::ranges::copy(srcRowData, destRowData.begin());

				currMipLevelUpdate.SetInputData(std::span<const std::uint8_t>{ alignedMipLevelByteArr });
			}

			textureUpdateArr.push_back(std::move(currMipLevelImage));
		}

		return LoadedSceneTextureInfo{
			.SceneTexture{ std::move(createdTexturePtr) },
			.SceneTextureUpdateArr{ std::move(textureUpdateArr) }
		};
	}
}

namespace Brawler
{
	void AssimpMaterialLoader::LoadMaterials(const aiScene& scene)
	{
		mMaterialBuilderArr.clear();

		const std::span<const aiMaterial*> materialPtrSpan{ scene.mMaterials, scene.mNumMaterials };
		CreateSceneTextures(materialPtrSpan);
	}

	std::span<const StandardMaterialBuilder> AssimpMaterialLoader::GetMaterialBuilderSpan() const
	{
		return std::span<const StandardMaterialBuilder>{ mMaterialBuilderArr };
	}

	void AssimpMaterialLoader::CreateSceneTextures(const std::span<const aiMaterial*> materialPtrSpan)
	{
		// It is possible that two materials use some of the same textures. We don't want to
		// bother loading this data twice, so instead, we create a std::unordered_set containing
		// a std::filesystem::path for each texture we find. We then create SceneTexture instances
		// for each texture.
		std::unordered_set<std::filesystem::path> textureFilePathSet{};
		std::unordered_set<FilePathHash> textureFilePathHashSet{};

		mMaterialBuilderArr.reserve(materialPtrSpan.size());

		for (const auto materialPtr : materialPtrSpan)
		{
			// For now, we assume that each aiMaterial represents a StandardMaterialDefinition
			// instance.

			aiString currTextureNameStr{};
			StandardMaterialBuilder materialBuilder{};

			const auto addFilePathLambda = [&currTextureNameStr, &textureFilePathSet, &textureFilePathHashSet] ()
			{
				std::filesystem::path textureFilePath{ currTextureNameStr.C_Str() };
				const FilePathHash textureFilePathHash{ Brawler::CONSTRUCT_FILE_PATH_HASH_AT_RUNTIME, textureFilePath.c_str() };

				textureFilePathSet.insert(std::move(textureFilePath));
				textureFilePathHashSet.insert(textureFilePathHash);

				return textureFilePathHash;
			};

			// Get the base color texture.
			if (materialPtr->GetTexture(aiTextureType::aiTextureType_DIFFUSE, 0, &currTextureNameStr) == aiReturn::aiReturn_SUCCESS) [[likely]]
				materialBuilder.SetBaseColorFilePathHash(addFilePathLambda());

			// Get the normal map texture.
			if (materialPtr->GetTexture(aiTextureType::aiTextureType_NORMALS, 0, &currTextureNameStr) == aiReturn::aiReturn_SUCCESS) [[likely]]
				materialBuilder.SetNormalMapFilePathHash(addFilePathLambda());

			// Get the roughness texture.
			if (materialPtr->GetTexture(aiTextureType::aiTextureType_SHININESS, 0, &currTextureNameStr) == aiReturn::aiReturn_SUCCESS) [[likely]]
				materialBuilder.SetRoughnessFilePathHash(addFilePathLambda());

			// Get the metallic texture.
			if (materialPtr->GetTexture(aiTextureType::aiTextureType_AMBIENT, 0, &currTextureNameStr) == aiReturn::aiReturn_SUCCESS) [[likely]]
				materialBuilder.SetMetallicFilePathHash(addFilePathLambda());

			mMaterialBuilderArr.push_back(std::move(materialBuilder));
		}

		std::vector<LoadedSceneTextureInfo> loadedTextureInfoArr{};
		loadedTextureInfoArr.resize(textureFilePathSet.size());

		Brawler::JobGroup sceneTextureLoadGroup{};
		sceneTextureLoadGroup.Reserve(textureFilePathSet.size());

		for (auto& [filePath, loadedTextureInfo] : std::views::zip(textureFilePathSet, loadedTextureInfoArr))
		{
			sceneTextureLoadGroup.AddJob([&filePath, &loadedTextureInfo] ()
			{
				loadedTextureInfo = LoadSceneTexture(filePath);
			});
		}

		sceneTextureLoadGroup.ExecuteJobs();

		// Now that the textures have been loaded, send them to the SceneTextureDatabase and
		// register the pre-frame texture updates with the GPUSceneUpdateRenderModule.
		for (auto& [filePathHash, loadedTextureInfo] : std::views::zip(textureFilePathHashSet, loadedTextureInfoArr))
		{
			SceneTextureDatabase::GetInstance().RegisterSceneTexture(filePathHash, std::move(loadedTextureInfo.SceneTexture));

			for (auto&& textureUpdate : loadedTextureInfo.SceneTextureUpdateArr)
				Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGPUSceneTextureUpdateForNextFrame(std::move(textureUpdate));
		}
	}
}