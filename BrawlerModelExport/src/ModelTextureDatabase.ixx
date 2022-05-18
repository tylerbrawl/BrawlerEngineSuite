module;
#include <tuple>
#include <vector>
#include <assimp/texture.h>
#include <DirectXTex.h>

export module Brawler.ModelTextureDatabase;
import Brawler.ModelTexture;
import Util.ModelTexture;
import Brawler.FilePathHash;
import Brawler.ModelTextureLUT;
import Brawler.Like_T;
import Brawler.LODScene;
import Brawler.ModelTextureHandle;
import Brawler.ImportedMesh;
import Brawler.JobGroup;
import Brawler.ModelTextureBuilderCollection;

#pragma push_macro("AddJob")
#undef AddJob

namespace Brawler
{
	// Rather than use dynamic polymorphism to hold a vector of ModelTexture objects
	// of different runtime types, we will create a std::tuple containing a vector
	// for each recognized aiTextureType.

	using RecognizedTextureTypeLUTTuple = std::tuple<
		ModelTextureLUT<aiTextureType::aiTextureType_DIFFUSE>
	>;

	template <aiTextureType TextureType>
	concept IsRecognizedTextureType = requires (RecognizedTextureTypeLUTTuple tuple)
	{
		std::get<ModelTextureLUT<TextureType>>(tuple);
	};
}

export namespace Brawler
{
	class ModelTextureDatabase final
	{
	private:
		ModelTextureDatabase() = default;

	public:
		~ModelTextureDatabase() = default;

		ModelTextureDatabase(const ModelTextureDatabase& rhs) = delete;
		ModelTextureDatabase& operator=(const ModelTextureDatabase& rhs) = delete;

		ModelTextureDatabase(ModelTextureDatabase&& rhs) noexcept = default;
		ModelTextureDatabase& operator=(ModelTextureDatabase&& rhs) noexcept = default;

		static ModelTextureDatabase& GetInstance();

		void CreateModelTextures(ModelTextureBuilderCollection& textureBuilderCollection);

		void InitializeScratchTextures();

		void UpdateModelTextures();
		bool AreModelTexturesReadyForSerialization() const;

	private:
		template <aiTextureType TextureType>
		ModelTextureLUT<TextureType>& GetModelTextureLUT();

		template <aiTextureType TextureType>
		const ModelTextureLUT<TextureType>& GetModelTextureLUT() const;

		template <typename Callback>
		void ForEachModelTextureLUT(const Callback& callback);

		template <typename Callback>
		void ForEachModelTextureLUT(const Callback& callback) const;

	private:
		RecognizedTextureTypeLUTTuple mLUTTuple;
	};
}

// -----------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	ModelTextureDatabase& ModelTextureDatabase::GetInstance()
	{
		static ModelTextureDatabase instance{};
		return instance;
	}

	void ModelTextureDatabase::CreateModelTextures(ModelTextureBuilderCollection& textureBuilderCollection)
	{
		ForEachModelTextureLUT([&textureBuilderCollection]<aiTextureType TextureType>(ModelTextureLUT<TextureType>&textureLUT)
		{
			textureLUT.CreateModelTextures(textureBuilderCollection);
		});
	}

	void ModelTextureDatabase::InitializeScratchTextures()
	{
		Brawler::JobGroup initScratchTexturesInLUTGroup{};
		initScratchTexturesInLUTGroup.Reserve(std::tuple_size_v<RecognizedTextureTypeLUTTuple>);

		ForEachModelTextureLUT([&initScratchTexturesInLUTGroup]<aiTextureType TextureType>(ModelTextureLUT<TextureType>&modelTextureLUT)
		{
			initScratchTexturesInLUTGroup.AddJob([&modelTextureLUT] ()
			{
				modelTextureLUT.InitializeScratchTextures();
			});
		});

		initScratchTexturesInLUTGroup.ExecuteJobs();
	}

	void ModelTextureDatabase::UpdateModelTextures()
	{
		Brawler::JobGroup modelTextureLUTUpdateGroup{};
		modelTextureLUTUpdateGroup.Reserve(std::tuple_size_v<RecognizedTextureTypeLUTTuple>);

		ForEachModelTextureLUT([&modelTextureLUTUpdateGroup]<aiTextureType TextureType>(ModelTextureLUT<TextureType>&modelTextureLUT)
		{
			modelTextureLUTUpdateGroup.AddJob([&modelTextureLUT] ()
			{
				modelTextureLUT.UpdateModelTextures();
			});
		});

		modelTextureLUTUpdateGroup.ExecuteJobs();
	}

	bool ModelTextureDatabase::AreModelTexturesReadyForSerialization() const
	{
		bool readyForSerialization = true;
		ForEachModelTextureLUT([&readyForSerialization]<aiTextureType TextureType>(const ModelTextureLUT<TextureType>&modelTextureLUT)
		{
			if (!modelTextureLUT.AreModelTexturesReadyForSerialization())
				readyForSerialization = false;
		});

		return readyForSerialization;
	}
	
	template <aiTextureType TextureType>
	ModelTextureLUT<TextureType>& ModelTextureDatabase::GetModelTextureLUT()
	{
		return std::get<ModelTextureLUT<TextureType>>(mLUTTuple);
	}

	template <aiTextureType TextureType>
	const ModelTextureLUT<TextureType>& ModelTextureDatabase::GetModelTextureLUT() const
	{
		return std::get<ModelTextureLUT<TextureType>>(mLUTTuple);
	}

	template <typename Callback>
	void ModelTextureDatabase::ForEachModelTextureLUT(const Callback& callback)
	{
		static constexpr auto EXECUTE_LAMBDA_CALLBACK = []<std::size_t CurrIndex>(this const auto& lambdaSelf, RecognizedTextureTypeLUTTuple& lutTuple, const Callback& callback)
		{
			if constexpr (CurrIndex != std::tuple_size_v<RecognizedTextureTypeLUTTuple>)
			{
				callback(std::get<CurrIndex>(lutTuple));
				lambdaSelf.operator()<(CurrIndex + 1)>(lutTuple, callback);
			}
		};

		EXECUTE_LAMBDA_CALLBACK.operator()<0>(mLUTTuple, callback);
	}

	template <typename Callback>
	void ModelTextureDatabase::ForEachModelTextureLUT(const Callback& callback) const
	{
		static constexpr auto EXECUTE_LAMBDA_CALLBACK = []<std::size_t CurrIndex>(this const auto& lambdaSelf, const RecognizedTextureTypeLUTTuple& lutTuple, const Callback& callback)
		{
			if constexpr (CurrIndex != std::tuple_size_v<RecognizedTextureTypeLUTTuple>)
			{
				callback(std::get<CurrIndex>(lutTuple));
				lambdaSelf.operator()<(CurrIndex + 1)>(lutTuple, callback);
			}
		};

		EXECUTE_LAMBDA_CALLBACK.operator()<0>(mLUTTuple, callback);
	}
}

#pragma pop_macro("AddJob")