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

		void RegisterModelTextureFromScene(const LODScene& scene);

		/// <summary>
		/// Constructs a ModelTextureHandle&lt;TextureType&gt; instance which refers to the corresponding
		/// ModelTexture instance based on the specified parameters. The textureIndex value refers to the
		/// index of the texture within the aiMaterial of mesh (i.e., 
		/// mesh.GetMeshMaterial.GetTexture(TextureType, textureIndex, ...)).
		/// 
		/// The function asserts in Debug builds if no such texture exists for that material. This will
		/// be the case if the texture search goes out of bounds.
		/// 
		/// *NOTE*: This function *IS* thread safe.
		/// </summary>
		/// <param name="mesh">
		/// - The ImportedMesh instance whose aiMaterial refers to the texture of instance. The
		///   ModelTextureDatabase resolves this reference to the corresponding ModelTexture instance.
		/// </param>
		/// <param name="textureIndex">
		/// - The index into the mMaterials list of mesh.GetMeshMaterials() at which the texture
		///   is to be found. By default, this value is zero (0).
		/// </param>
		/// <returns>
		/// The function returns a ModelTextureHandle&lt;TextureType&gt; instance which refers to the
		/// corresponding ModelTexture instance based on the specified parameters.
		/// </returns>
		template <aiTextureType TextureType>
			requires IsRecognizedTextureType<TextureType>
		ModelTextureHandle<TextureType> GetModelTextureHandle(const ImportedMesh& mesh, const std::size_t textureIndex = 0) const;

	private:
		template <aiTextureType TextureType>
		ModelTextureLUT<TextureType>& GetModelTextureLUT();

		template <aiTextureType TextureType>
		const ModelTextureLUT<TextureType>& GetModelTextureLUT() const;

		template <typename Callback>
		void ForEachModelTextureLUT(const Callback& callback);

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
	
	void ModelTextureDatabase::RegisterModelTextureFromScene(const LODScene& scene)
	{
		ForEachModelTextureLUT([&scene]<aiTextureType TextureType>(ModelTextureLUT<TextureType>& textureLUT)
		{
			textureLUT.AddTexturesFromScene(scene);
		});
	}

	template <aiTextureType TextureType>
		requires IsRecognizedTextureType<TextureType>
	ModelTextureHandle<TextureType> ModelTextureDatabase::GetModelTextureHandle(const ImportedMesh& mesh, const std::size_t textureIndex) const
	{
		return GetModelTextureLUT<TextureType>().GetModelTextureHandle(mesh, textureIndex);
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
}