module;
#include <unordered_map>
#include <memory>
#include <mutex>
#include <cassert>

export module Brawler.AssetManager;
import Brawler.AssetHandle;
import Brawler.I_Asset;
import Brawler.FilePathHash;
import Brawler.AssetTypeMap;
import Brawler.AssetTypeID;
import Brawler.BPKArchiveReader;
import Brawler.AssetDependencyManager;
import Brawler.AssetDependencyGroup;
import Brawler.PersistentAssetDependencyGroup;
import Brawler.AssetDataRequests;
import Brawler.JobPriority;
import Brawler.AssetDependencyTracker;

export namespace Brawler
{
	class AssetManager
	{
	public:
		AssetManager();

		AssetManager(const AssetManager& rhs) = delete;
		AssetManager& operator=(const AssetManager& rhs) = delete;

		AssetManager(AssetManager&& rhs) noexcept = default;
		AssetManager& operator=(AssetManager&& rhs) noexcept = default;

		template <typename T>
			requires std::derived_from<T, I_Asset>
		AssetHandle<T> CreateAssetHandle(const FilePathHash& pathHash);

		void UpdateAssetDependencies();

		BPKArchiveReader& GetBPKArchiveReader();
		const BPKArchiveReader& GetBPKArchiveReader() const;

		AssetDependencyTracker SubmitAssetDependencyGroup(AssetDependencyGroup&& dependencyGroup);
		AssetDependencyTracker SubmitAssetDependencyGroup(PersistentAssetDependencyGroup&& persistentGroup);

		void SubmitAssetDataLoadRequest(std::unique_ptr<AssetDataLoadRequest>&& loadRequest, const JobPriority priority);
		void SubmitAssetDataUnloadRequest(std::unique_ptr<AssetDataUnloadRequest>&& unloadRequest, const JobPriority priority);

		std::uint64_t GetCurrentUpdateTick() const;

	private:
		/// <summary>
		/// This is a map between file path hash values (stored as their raw
		/// std::uint64_t values) and unique I_Asset instances. It is used to ensure
		/// that only one I_Asset instance exists for every asset within a BPK
		/// archive.
		/// </summary>
		std::unordered_map<std::uint64_t, std::unique_ptr<I_Asset>> mHashAssetMap;

		AssetDependencyManager mDependencyManager;
		bool mIsUpdating;
		std::uint64_t mCurrUpdateTick;
		BPKArchiveReader mBPKReader;
		mutable std::mutex mHashAssetMapCritSection;
	};
}

// -----------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
		requires std::derived_from<T, I_Asset>
	AssetHandle<T> AssetManager::CreateAssetHandle(const FilePathHash& pathHash)
	{
		// Make sure that we are not creating an AssetHandle of an inappropriate type.
		static_assert(AssetTypeMap::GetAssetTypeID<T>() != AssetTypeID::COUNT_OR_ERROR, "ERROR: An attempt was made to create an AssetHandle with an inappropriate type!");
		
		std::scoped_lock<std::mutex> lock{ mHashAssetMapCritSection };

		if (mHashAssetMap.contains(pathHash.GetHash()))
		{
			// Before we do the cast, make sure that we remain consistent with our types.
			I_Asset* const assetPtr = mHashAssetMap.at(pathHash.GetHash()).get();
			assert(assetPtr->GetAssetTypeID() == AssetTypeMap::GetAssetTypeID<T>() && "ERROR: An attempt was made to create an AssetHandle using an existing I_Asset instance, but the wrong type was provided!");

			return AssetHandle<T>{ *(static_cast<T*>(assetPtr)) };
		}
		
		// If no I_Asset instance has been created yet for this particular asset, then create
		// one with the specified underlying type. When AssetManager::CreateAssetHandle() is
		// called again with the same file path hash, it will re-use the created I_Asset instance
		// instead.

		std::unique_ptr<I_Asset> assetPtr{ std::make_unique<T>(FilePathHash{ pathHash }) };

		// Perform the initialization steps which are required for proper initialization of
		// an I_Asset.
		assetPtr->SetOwningManager(*this);

		T& assetReference = *(static_cast<T*>(assetPtr.get()));

		mHashAssetMap[pathHash.GetHash()] = std::move(assetPtr);

		return AssetHandle<T>{ assetReference };
	}
}