module;
#include <unordered_map>
#include <memory>

module Brawler.AssetManager;

namespace Brawler
{
	AssetManager::AssetManager() :
		mHashAssetMap(),
		mDependencyManager(*this),
		mCurrUpdateTick(0),
		mIsUpdating(false),
		mBPKReader(),
		mHashAssetMapCritSection()
	{}

	void AssetManager::UpdateAssetDependencies()
	{
		mIsUpdating = true;
		
		mDependencyManager.UpdateAssetDependencies();

		++mCurrUpdateTick;
		mIsUpdating = false;
	}

	BPKArchiveReader& AssetManager::GetBPKArchiveReader()
	{
		return mBPKReader;
	}

	const BPKArchiveReader& AssetManager::GetBPKArchiveReader() const
	{
		return mBPKReader;
	}

	AssetDependencyTracker AssetManager::SubmitAssetDependencyGroup(AssetDependencyGroup&& dependencyGroup)
	{
		std::unique_ptr<AssetDependencyGroup> groupPtr{ std::make_unique<AssetDependencyGroup>(std::move(dependencyGroup)) };
		const AssetDependencyGroup* const rawGroupPtr = groupPtr.get();
		mDependencyManager.SubmitAssetDependencyGroup(std::move(groupPtr));

		return AssetDependencyTracker{ *rawGroupPtr };
	}

	AssetDependencyTracker AssetManager::SubmitAssetDependencyGroup(PersistentAssetDependencyGroup&& persistentGroup)
	{
		std::unique_ptr<PersistentAssetDependencyGroup> groupPtr{ std::make_unique<PersistentAssetDependencyGroup>(std::move(persistentGroup)) };
		const PersistentAssetDependencyGroup* const rawGroupPtr = groupPtr.get();
		mDependencyManager.SubmitAssetDependencyGroup(std::move(groupPtr));

		return AssetDependencyTracker{ *rawGroupPtr };
	}

	void AssetManager::SubmitAssetDataLoadRequest(std::unique_ptr<AssetDataLoadRequest>&& loadRequest, const JobPriority priority)
	{
		mDependencyManager.SubmitAssetDataLoadRequest(std::move(loadRequest), priority);
	}

	void AssetManager::SubmitAssetDataUnloadRequest(std::unique_ptr<AssetDataUnloadRequest>&& unloadRequest, const JobPriority priority)
	{
		mDependencyManager.SubmitAssetDataUnloadRequest(std::move(unloadRequest), priority);
	}

	std::uint64_t AssetManager::GetCurrentUpdateTick() const
	{
		return mCurrUpdateTick;
	}
}