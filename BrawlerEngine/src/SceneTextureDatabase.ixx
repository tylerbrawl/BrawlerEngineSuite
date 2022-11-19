module;
#include <optional>
#include <tuple>
#include <unordered_map>

export module Brawler.SceneTextureDatabase;
import :TypedSceneTextureDatabase;
import Brawler.SceneTextures;
import Brawler.OptionalRef;
import Brawler.FilePathHash;
import Brawler.SceneTextureHandles;
import Brawler.MaterialDefinitionGraph;

export namespace Brawler
{
	class SceneTextureDatabase final
	{
	private:
		using TypedDatabaseTuple_T = std::tuple<
			TypedSceneTextureDatabase<SceneTexture2D>
		>;

	private:
		template <typename SceneTextureType>
		friend class SceneTextureHandle<SceneTextureType>;

	private:
		SceneTextureDatabase() = default;

	public:
		~SceneTextureDatabase() = default;

		SceneTextureDatabase(const SceneTextureDatabase& rhs) = delete;
		SceneTextureDatabase& operator=(const SceneTextureDatabase& rhs) = delete;

		SceneTextureDatabase(SceneTextureDatabase&& rhs) noexcept = delete;
		SceneTextureDatabase& operator=(SceneTextureDatabase&& rhs) noexcept = delete;

		static SceneTextureDatabase& GetInstance();

		template <typename SceneTextureType>
		void RegisterSceneTexture(const FilePathHash pathHash, SceneTextureType&& sceneTexture);

		template <typename SceneTextureType>
		std::optional<SceneTextureHandle<SceneTextureType>> CreateSceneTextureHandle(const FilePathHash pathHash);

		void DeleteUnreferencedSceneTextures();

	private:
		template <typename SceneTextureType>
		std::optional<std::uint32_t> GetSceneTextureSRVIndex(const FilePathHash pathHash) const;

		template <typename SceneTextureType>
		void DecrementSceneTextureReferenceCount(const FilePathHash pathHash);

		template <typename SceneTextureType, typename Callback>
		auto ExecuteCallbackForTypedDatabase(const Callback& callback);

		template <typename SceneTextureType, typename Callback>
		auto ExecuteCallbackForTypedDatabase(const Callback& callback) const;

	private:
		TypedDatabaseTuple_T mDatabaseTuple;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename SceneTextureType>
	void SceneTextureDatabase::RegisterSceneTexture(const FilePathHash pathHash, SceneTextureType&& sceneTexture)
	{
		ExecuteCallbackForTypedDatabase<SceneTextureType>([pathHash, &sceneTexture] (TypedSceneTextureDatabase<SceneTextureType>& database)
		{
			database.RegisterSceneTexture(pathHash, std::move(sceneTexture));
		});

		// Upon modifying a scene texture, we should notify the MaterialDefinitionGraph that
		// any I_MaterialDefinition instances using said texture will need to update the
		// material description in the GPUSceneBuffers.
		MaterialDefinitionGraph::GetInstance().RequestGPUSceneMaterialDescriptorUpdate(pathHash);
	}

	template <typename SceneTextureType>
	std::optional<SceneTextureHandle<SceneTextureType>> SceneTextureDatabase::CreateSceneTextureHandle(const FilePathHash pathHash)
	{
		return ExecuteCallbackForTypedDatabase<SceneTextureType>([pathHash] (TypedSceneTextureDatabase<SceneTextureType>& database)
		{
			return database.CreateSceneTextureHandle(pathHash);
		});
	}

	template <typename SceneTextureType>
	std::optional<std::uint32_t> SceneTextureDatabase::GetSceneTextureSRVIndex(const FilePathHash pathHash) const
	{
		return ExecuteCallbackForTypedDatabase<SceneTextureType>([pathHash] (const TypedSceneTextureDatabase<SceneTextureType>& database)
		{
			return database.GetSceneTextureSRVIndex(pathHash);
		});
	}

	template <typename SceneTextureType>
	void SceneTextureDatabase::DecrementSceneTextureReferenceCount(const FilePathHash pathHash)
	{
		ExecuteCallbackForTypedDatabase<SceneTextureType>([pathHash] (TypedSceneTextureDatabase<SceneTextureType>& database)
		{
			database.DecrementSceneTextureReferenceCount(pathHash);
		});
	}

	template <typename SceneTextureType, typename Callback>
	auto SceneTextureDatabase::ExecuteCallbackForTypedDatabase(const Callback& callback)
	{
		const auto executeCallbackLambda = [this, &callback]<std::size_t CurrIndex>(this const auto& self)
		{
			using CurrDatabaseType = std::tuple_element_t<CurrIndex, TypedDatabaseTuple_T>;

			if constexpr (std::is_same_v<CurrDatabaseType, TypedSceneTextureDatabase<SceneTextureType>>)
				return callback(std::get<CurrIndex>(mDatabaseTuple));

			else
			{
				static constexpr std::size_t NEXT_INDEX = (CurrIndex + 1);

				if constexpr (NEXT_INDEX != std::tuple_size_v<TypedDatabaseTuple_T>)
					return self.template operator()<NEXT_INDEX>();

				else
				{
					static_assert(sizeof(SceneTextureType) != sizeof(SceneTextureType));
					std::unreachable();

					return callback(std::get<0>(mDatabaseTuple));
				}
			}
		};

		executeCallbackLambda.template operator()<0>();
	}

	template <typename SceneTextureType, typename Callback>
	auto SceneTextureDatabase::ExecuteCallbackForTypedDatabase(const Callback& callback) const
	{
		const auto executeCallbackLambda = [this, &callback]<std::size_t CurrIndex>(this const auto& self)
		{
			using CurrDatabaseType = std::tuple_element_t<CurrIndex, TypedDatabaseTuple_T>;

			if constexpr (std::is_same_v<CurrDatabaseType, TypedSceneTextureDatabase<SceneTextureType>>)
				return callback(std::get<CurrIndex>(mDatabaseTuple));

			else
			{
				static constexpr std::size_t NEXT_INDEX = (CurrIndex + 1);

				if constexpr (NEXT_INDEX != std::tuple_size_v<TypedDatabaseTuple_T>)
					return self.template operator()<NEXT_INDEX>();

				else
				{
					static_assert(sizeof(SceneTextureType) != sizeof(SceneTextureType));
					std::unreachable();

					return callback(std::get<0>(mDatabaseTuple));
				}
			}
		};

		executeCallbackLambda.template operator()<0>();
	}
}