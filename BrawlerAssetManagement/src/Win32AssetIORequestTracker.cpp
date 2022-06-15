module;
#include <atomic>

module Brawler.AssetManagement.Win32AssetIORequestTracker;

namespace Brawler
{
	namespace AssetManagement
	{
		Win32AssetIORequestTracker::Win32AssetIORequestTracker(AssetRequestEventHandle&& hAssetRequestEvent) :
			mHAssetRequestEvent(std::move(hAssetRequestEvent)),
			mActiveRequestCounter(0)
		{}

		void Win32AssetIORequestTracker::SetActiveRequestCount(const std::uint32_t numActiveRequests)
		{
			if(numActiveRequests > 0) [[likely]]
				mActiveRequestCounter.store(numActiveRequests, std::memory_order::relaxed);
			else [[unlikely]]
				AssetRequestEventNotifier::MarkAssetRequestAsCompleted(mHAssetRequestEvent);
		}

		void Win32AssetIORequestTracker::NotifyForAssetIORequestCompletion()
		{
			const std::uint32_t numRequestsRemaining = (mActiveRequestCounter.fetch_sub(1, std::memory_order::relaxed) - 1);

			if (numRequestsRemaining == 0)
				AssetRequestEventNotifier::MarkAssetRequestAsCompleted(mHAssetRequestEvent);
		}

		bool Win32AssetIORequestTracker::IsAssetRequestEventComplete() const
		{
			return mHAssetRequestEvent.IsAssetRequestComplete();
		}
	}
}