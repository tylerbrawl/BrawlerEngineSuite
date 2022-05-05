module;
#include <utility>
#include <atomic>
#include <memory>
#include <cassert>

module Brawler.I_Asset;
import Brawler.AssetManager;
import Brawler.AssetDataRequests;

namespace Brawler
{
	I_Asset::I_Asset(FilePathHash&& pathHash) :
		mPathHash(std::move(pathHash)),
		mOwningManager(nullptr),
		mUpdateTickCounter(0)
	{}

	bool I_Asset::IsStreamable() const
	{
		return true;
	}

	void I_Asset::SubmitAssetDataLoadRequest(const std::size_t numBytesToLoad, const JobPriority priority)
	{
		std::unique_ptr<AssetDataLoadRequest> loadRequest{ std::make_unique<AssetDataLoadRequest>(*this, numBytesToLoad) };
		GetAssetManager().SubmitAssetDataLoadRequest(std::move(loadRequest), priority);
	}

	void I_Asset::SubmitAssetDataUnloadRequest(const std::size_t numBytesToUnload, const JobPriority priority)
	{
		std::unique_ptr<AssetDataUnloadRequest> unloadRequest{ std::make_unique<AssetDataUnloadRequest>(*this, numBytesToUnload) };
		GetAssetManager().SubmitAssetDataUnloadRequest(std::move(unloadRequest), priority);
	}

	FilePathHash I_Asset::GetFilePathHash() const
	{
		return mPathHash;
	}

	AssetManager& I_Asset::GetAssetManager()
	{
		assert(mOwningManager != nullptr && "ERROR: An I_Asset instance was not assigned an AssetManager before calling I_Asset::GetAssetManager()!");
		return *mOwningManager;
	}

	const AssetManager& I_Asset::GetAssetManager() const
	{
		assert(mOwningManager != nullptr && "ERROR: An I_Asset instance was not assigned an AssetManager before calling I_Asset::GetAssetManager()!");
		return *mOwningManager;
	}

	void I_Asset::UpdateAssetDataIMPL()
	{
		// Since an I_Asset instance can be referenced in more than one asset dependency group,
		// we need some way to make sure that requests are only ever sent from an I_Asset instance
		// once per update tick. (mUpdateTickCounter is atomic because at the time of writing this,
		// asset data updates are multi-threaded. This design is subject to change based on
		// performance under load.)

		std::uint64_t currUpdateTick = GetAssetManager().GetCurrentUpdateTick();

		if (mUpdateTickCounter.compare_exchange_strong(currUpdateTick, (currUpdateTick + 1)))
			UpdateAssetData();
	}

	void I_Asset::SetOwningManager(AssetManager& owningManager)
	{
		mOwningManager = &owningManager;
	}
}