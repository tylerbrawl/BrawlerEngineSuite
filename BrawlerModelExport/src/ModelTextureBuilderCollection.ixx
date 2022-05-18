module;
#include <memory>
#include <vector>
#include <tuple>
#include <span>
#include <assimp/material.h>

export module Brawler.ModelTextureBuilderCollection;
import Brawler.I_ModelTextureBuilder;
import Brawler.ModelTextureHandle;

namespace Brawler
{
	template <aiTextureType TextureType>
	using ModelTextureBuilderArray = std::vector<std::unique_ptr<I_ModelTextureBuilder<TextureType>>>;
	
	using ModelTextureBuilderTupleType = std::tuple<
		ModelTextureBuilderArray<aiTextureType::aiTextureType_DIFFUSE>
	>;

	template <aiTextureType TextureType>
	concept IsRecognizedTextureType = requires (ModelTextureBuilderTupleType tuple)
	{
		std::get<ModelTextureBuilderArray<TextureType>>(tuple);
	};
}

export namespace Brawler
{
	class ModelTextureBuilderCollection
	{
	public:
		ModelTextureBuilderCollection() = default;

		ModelTextureBuilderCollection(const ModelTextureBuilderCollection& rhs) = delete;
		ModelTextureBuilderCollection& operator=(const ModelTextureBuilderCollection& rhs) = delete;

		ModelTextureBuilderCollection(ModelTextureBuilderCollection&& rhs) noexcept = default;
		ModelTextureBuilderCollection& operator=(ModelTextureBuilderCollection&& rhs) noexcept = default;

		template <aiTextureType TextureType>
			requires IsRecognizedTextureType<TextureType>
		ModelTextureHandle<TextureType> AddModelTextureBuilder(std::unique_ptr<I_ModelTextureBuilder<TextureType>>&& builderPtr);

		template <aiTextureType TextureType>
			requires IsRecognizedTextureType<TextureType>
		std::span<std::unique_ptr<I_ModelTextureBuilder<TextureType>>> GetModelTextureBuilderSpan();

		void MergeModelTextureBuilderCollection(ModelTextureBuilderCollection&& srcCollection);

	private:
		template <std::size_t CurrIndex>
		void MergeModelTextureBuilderCollectionIMPL(ModelTextureBuilderCollection&& srcCollection);

	private:
		ModelTextureBuilderTupleType mBuilderArrTuple;
	};
}

// -------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <std::size_t CurrIndex>
	void ModelTextureBuilderCollection::MergeModelTextureBuilderCollectionIMPL(ModelTextureBuilderCollection&& srcCollection)
	{
		if constexpr (CurrIndex != std::tuple_size_v<ModelTextureBuilderTupleType>)
		{
			auto& srcTextureBuilderArr{ std::get<CurrIndex>(srcCollection.mBuilderArrTuple) };

			for (auto&& builderPtr : srcTextureBuilderArr)
				AddModelTextureBuilder(std::move(builderPtr));

			MergeModelTextureBuilderCollectionIMPL<(CurrIndex + 1)>(std::move(srcCollection));
		}
	}

	template <aiTextureType TextureType>
		requires IsRecognizedTextureType<TextureType>
	ModelTextureHandle<TextureType> ModelTextureBuilderCollection::AddModelTextureBuilder(std::unique_ptr<I_ModelTextureBuilder<TextureType>>&& builderPtr)
	{
		auto& textureBuilderArr{ std::get<ModelTextureBuilderArray<TextureType>>(mBuilderArrTuple) };

		// Check to see if any existing textures are duplicates of the texture represented by builderPtr.
		// If so, then we do not insert the provided I_ModelTextureBuilder instance into the respective
		// array.
		for (const auto& existingBuilderPtr : textureBuilderArr)
		{
			if (existingBuilderPtr->IsDuplicateTexture(*builderPtr)) [[unlikely]]
				return ModelTextureHandle<TextureType>{ *existingBuilderPtr };
		}

		const ModelTextureHandle<TextureType> hModelTexture{ *builderPtr };
		textureBuilderArr.push_back(std::move(builderPtr));

		return hModelTexture;
	}

	template <aiTextureType TextureType>
		requires IsRecognizedTextureType<TextureType>
	std::span<std::unique_ptr<I_ModelTextureBuilder<TextureType>>> ModelTextureBuilderCollection::GetModelTextureBuilderSpan()
	{
		return std::span<std::unique_ptr<I_ModelTextureBuilder<TextureType>>>{ std::get<ModelTextureBuilderArray<TextureType>>(mBuilderArrTuple) };
	}

	void ModelTextureBuilderCollection::MergeModelTextureBuilderCollection(ModelTextureBuilderCollection&& srcCollection)
	{
		MergeModelTextureBuilderCollectionIMPL<0>(std::move(srcCollection));
	}
}