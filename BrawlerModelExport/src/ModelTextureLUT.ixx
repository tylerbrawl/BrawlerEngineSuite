module;
#include <vector>
#include <memory>
#include <unordered_map>
#include <span>
#include <cassert>
#include <filesystem>
#include <format>
#include <assimp/scene.h>
#include <DirectXTex.h>

export module Brawler.ModelTextureLUT;
import Brawler.ModelTexture;
import Brawler.LODScene;
import Util.ModelTexture;
import Brawler.FilePathHash;
import Util.General;
import Brawler.ModelTextureHandle;
import Brawler.ImportedMesh;
import Brawler.JobGroup;
import Brawler.ModelTextureBuilderCollection;
import Brawler.I_ModelTextureBuilder;

#pragma push_macro("AddJob")
#undef AddJob

export namespace Brawler
{
	template <aiTextureType TextureType>
	class ModelTextureLUT
	{
	private:
		using ModelTextureType = ModelTexture<TextureType>;

	public:
		ModelTextureLUT() = default;

		ModelTextureLUT(const ModelTextureLUT& rhs) = delete;
		ModelTextureLUT& operator=(const ModelTextureLUT& rhs) = delete;

		ModelTextureLUT(ModelTextureLUT&& rhs) noexcept = default;
		ModelTextureLUT& operator=(ModelTextureLUT&& rhs) noexcept = default;

		void CreateModelTextures(ModelTextureBuilderCollection& textureBuilderCollection);

		void InitializeScratchTextures();

		std::size_t GetTextureCount() const;

		void UpdateModelTextures();
		bool AreModelTexturesReadyForSerialization() const;

	private:
		std::vector<std::unique_ptr<ModelTextureType>> mModelTexturePtrArr;
		std::unordered_map<FilePathHash, ModelTextureType*> mTextureNameMap;
	};
}

// -------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
	void ModelTextureLUT<TextureType>::CreateModelTextures(ModelTextureBuilderCollection& textureBuilderCollection)
	{
		auto textureBuilderPtrSpan{ textureBuilderCollection.GetModelTextureBuilderSpan<TextureType>() };

		for (auto&& textureBuilderPtr : textureBuilderPtrSpan)
		{
			FilePathHash uniqueTextureNameHash{ textureBuilderPtr->GetUniqueTextureName() };
			assert(!mTextureNameMap.contains(uniqueTextureNameHash) && "ERROR: Two or more textures had the same FilePathHash of their unique texture names!");

			std::unique_ptr<ModelTextureType> modelTexture{ std::make_unique<ModelTextureType>(std::move(textureBuilderPtr)) };

			mTextureNameMap[uniqueTextureNameHash] = modelTexture.get();
			mModelTexturePtrArr.push_back(std::move(modelTexture));
		}
	}

	template <aiTextureType TextureType>
	void ModelTextureLUT<TextureType>::InitializeScratchTextures()
	{
		Brawler::JobGroup initScratchTexturesGroup{};
		initScratchTexturesGroup.Reserve(mModelTexturePtrArr.size());

		for (const auto& modelTexture : mModelTexturePtrArr)
			initScratchTexturesGroup.AddJob([modelTexturePtr = modelTexture.get()] ()
			{
				modelTexturePtr->GenerateIntermediateScratchTexture();
			});

		initScratchTexturesGroup.ExecuteJobs();
	}

	template <aiTextureType TextureType>
	std::size_t ModelTextureLUT<TextureType>::GetTextureCount() const
	{
		return mModelTexturePtrArr.size();
	}

	template <aiTextureType TextureType>
	void ModelTextureLUT<TextureType>::UpdateModelTextures()
	{
		Brawler::JobGroup textureUpdateGroup{};
		textureUpdateGroup.Reserve(mModelTexturePtrArr.size());

		for (const auto& modelTexture : mModelTexturePtrArr)
			textureUpdateGroup.AddJob([modelTexturePtr = modelTexture.get()] ()
			{
				modelTexturePtr->Update();
			});

		textureUpdateGroup.ExecuteJobs();
	}

	template <aiTextureType TextureType>
	bool ModelTextureLUT<TextureType>::AreModelTexturesReadyForSerialization() const
	{
		for (const auto& modelTexturePtr : mModelTexturePtrArr)
		{
			if (!modelTexturePtr->IsReadyForSerialization())
				return false;
		}

		return true;
	}
}

#pragma pop_macro("AddJob")