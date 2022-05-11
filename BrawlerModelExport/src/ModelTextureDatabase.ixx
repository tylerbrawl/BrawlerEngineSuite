module;
#include <tuple>
#include <vector>
#include <assimp/texture.h>

export module Brawler.ModelTextureDatabase;
import Brawler.ModelTexture;
import Util.ModelTexture;
import Brawler.FilePathHash;

namespace Brawler
{
	// Rather than use dynamic polymorphism to hold a vector of ModelTexture objects
	// of different runtime types, we will create a std::tuple containing a vector
	// for each recognized aiTextureType.

	using RecognizedTextureTypeTuple = std::tuple<
		ModelTexture<aiTextureType::aiTextureType_DIFFUSE>
	>;

	template <aiTextureType TextureType>
	concept IsRecognizedTextureType = requires (RecognizedTextureTypeTuple tuple)
	{
		std::get<ModelTexture<TextureType>>(tuple);
	};

	template <typename T>
	struct TextureTypeTupleSolver
	{
		static_assert(sizeof(T) != sizeof(T));
	};

	template <typename... TupleTypes>
	struct TextureTypeTupleSolver<std::tuple<TupleTypes...>>
	{
		using VectorTupleType = std::tuple<std::vector<TupleTypes>...>;
	};

	using RecognizedTextureTypeArraysTuple = typename TextureTypeTupleSolver<RecognizedTextureTypeTuple>::VectorTupleType;
}

export namespace Brawler
{
	class ModelTextureDatabase
	{
	public:
		ModelTextureDatabase() = default;

		ModelTextureDatabase(const ModelTextureDatabase& rhs) = delete;
		ModelTextureDatabase& operator=(const ModelTextureDatabase& rhs) = delete;

		ModelTextureDatabase(ModelTextureDatabase&& rhs) noexcept = default;
		ModelTextureDatabase& operator=(ModelTextureDatabase&& rhs) noexcept = default;

		template <aiTextureType TextureType>
			requires IsRecognizedTextureType<TextureType>
		ModelTexture<TextureType>& RegisterModelTexture(const aiString& textureName);

	private:
		template <aiTextureType TextureType, typename Self>
		std::vector<ModelTexture<TextureType>>& GetModelTextureArray(this Self&& self);

	private:
		RecognizedTextureTypeArraysTuple mTextureArrayTuple;
	};
}

// -----------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
		requires IsRecognizedTextureType<TextureType>
	ModelTexture<TextureType>& ModelTextureDatabase::RegisterModelTexture(const aiString& textureName)
	{
		std::vector<ModelTexture<TextureType>>& modelTextureArr{ GetModelTextureArray<TextureType>() };
		const FilePathHash texturePathHash{ Util::ModelTexture::GetTextureFilePathHash(textureName) };

		for (auto& texture : modelTextureArr)
		{
			if (texture.GetOutputPathHash() == texturePathHash)
				return texture;
		}

		modelTextureArr.emplace_back(textureName);
		return modelTextureArr.back();
	}
	
	template <aiTextureType TextureType, typename Self>
	std::vector<ModelTexture<TextureType>>& ModelTextureDatabase::GetModelTextureArray(this Self&& self)
	{
		return std::get<std::vector<ModelTexture<TextureType>>>(std::forward<Self>(self).mTextureArrayTuple);
	}
}