module;
#include <memory>
#include <atomic>
#include <cassert>

module Brawler.AssetManagement.AssetRequestEventHandle;

namespace Brawler
{
	namespace AssetManagement
	{
		AssetRequestEventHandle::AssetRequestEventHandle() :
			mAssetRequestFinished(std::make_shared<std::atomic<bool>>(false))
		{}
		
		bool AssetRequestEventHandle::IsAssetRequestComplete() const
		{
			// Do a read-acquire so that any threads writing asset data have their changes
			// propagated after checking if the request is completed.
			assert(mAssetRequestFinished != nullptr);
			return mAssetRequestFinished->load(std::memory_order::acquire);
		}

		void AssetRequestEventHandle::MarkAssetRequestAsCompleted()
		{
			// Perform a write-release to ensure that any changes which we made to asset
			// data are propagated to other threads.
			assert(mAssetRequestFinished != nullptr);
			mAssetRequestFinished->store(true, std::memory_order::release);
		}
	}
}