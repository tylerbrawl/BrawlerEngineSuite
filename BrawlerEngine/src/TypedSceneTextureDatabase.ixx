module;
#include <atomic>
#include <cassert>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

export module Brawler.SceneTextureDatabase:TypedSceneTextureDatabase;
import Brawler.SceneTextures;
import Brawler.SceneTextureHandles;
import Brawler.FilePathHash;
import Brawler.OptionalRef;
import Brawler.ScopedSharedLock;

export namespace Brawler
{
	template <typename SceneTextureType>
	class TypedSceneTextureDatabase
	{
	private:
		struct StoredTexture
		{
			SceneTextureType SceneTexture;
			std::atomic<std::uint64_t> ReferenceCount;
		};

	public:
		TypedSceneTextureDatabase() = default;

		TypedSceneTextureDatabase(const TypedSceneTextureDatabase& rhs) = delete;
		TypedSceneTextureDatabase& operator=(const TypedSceneTextureDatabase& rhs) = delete;

		TypedSceneTextureDatabase(TypedSceneTextureDatabase&& rhs) noexcept = delete;
		TypedSceneTextureDatabase& operator=(TypedSceneTextureDatabase&& rhs) noexcept = delete;

		void RegisterSceneTexture(const FilePathHash texturePathHash, SceneTextureType&& sceneTexture);

		std::optional<SceneTextureHandle<SceneTextureType>> CreateSceneTextureHandle(const FilePathHash texturePathHash);
		Brawler::OptionalRef<const SceneTextureType> GetSceneTexture(const FilePathHash texturePathHash) const;

		void DecrementSceneTextureReferenceCount(const FilePathHash texturePathHash);
		void DeleteUnreferencedSceneTextures();

	private:
		std::unordered_map<FilePathHash, StoredTexture> mSceneTextureMap;
		mutable std::shared_mutex mCritSection;
	};
}

// ------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename SceneTextureType>
	void TypedSceneTextureDatabase<SceneTextureType>::RegisterSceneTexture(const FilePathHash texturePathHash, SceneTextureType&& sceneTexture)
	{
		const Brawler::ScopedSharedWriteLock<std::shared_mutex> lock{ mCritSection };

		mSceneTextureMap[texturePathHash].SceneTexture = std::move(sceneTexture);
	}

	template <typename SceneTextureType>
	std::optional<SceneTextureHandle<SceneTextureType>> TypedSceneTextureDatabase<SceneTextureType>::CreateSceneTextureHandle(const FilePathHash texturePathHash)
	{
		const Brawler::ScopedSharedReadLock<std::shared_mutex> lock{ mCritSection };

		if (!mSceneTextureMap.contains(texturePathHash)) [[unlikely]]
			return {};

		StoredTexture& relevantStoredTexture{ mSceneTextureMap.at(texturePathHash) };

		// Increment the SceneTexture's reference count.
		relevantStoredTexture.ReferenceCount.fetch_add(1, std::memory_order::relaxed);

		return SceneTextureHandle<SceneTextureType>{ texturePathHash };
	}

	template <typename SceneTextureType>
	Brawler::OptionalRef<const SceneTextureType> TypedSceneTextureDatabase<SceneTextureType>::GetSceneTexture(const FilePathHash texturePathHash) const
	{
		const Brawler::ScopedSharedReadLock<std::shared_mutex> lock{ mCritSection };

		if (!mSceneTextureMap.contains(texturePathHash)) [[unlikely]]
			return {};

		// Since iterators and references within a std::unordered_map instance for a given
		// entry are only ever invalidated when said entry is erased from the map, and since
		// we don't erase entries from the map until we know that there are no SceneTextureHandle
		// instances referring to it, it is safe to return a const& to a SceneTextureType
		// instance not wrapped within a std::unique_ptr. This reduces heap allocation count.
		return Brawler::OptionalRef<const SceneTextureType>{ mSceneTextureMap.at(texturePathHash).SceneTexture };
	}

	template <typename SceneTextureType>
	void TypedSceneTextureDatabase<SceneTextureType>::DecrementSceneTextureReferenceCount(const FilePathHash texturePathHash)
	{
		const Brawler::ScopedSharedReadLock<std::shared_mutex> lock{ mCritSection };

		assert(mSceneTextureMap.contains(texturePathHash));

		[[maybe_unused]] const std::uint64_t prevRefCount = mSceneTextureMap.at(texturePathHash).fetch_sub(1, std::memory_order::relaxed);
		assert(prevRefCount != 0);
	}

	template <typename SceneTextureType>
	void TypedSceneTextureDatabase<SceneTextureType>::DeleteUnreferencedSceneTextures()
	{
		const Brawler::ScopedSharedWriteLock<std::shared_mutex> lock{ mCritSection };

		std::erase_if(mSceneTextureMap, [] (const auto& mapPair)
		{
			const auto& [hash, storedTexture] = mapPair;
			return (storedTexture.ReferenceCount.load(std::memory_order::relaxed) == 0);
		});
	}
}