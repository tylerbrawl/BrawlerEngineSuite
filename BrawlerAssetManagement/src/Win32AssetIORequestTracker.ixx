module;
#include <atomic>

export module Brawler.AssetManagement.Win32AssetIORequestTracker;
import Brawler.AssetManagement.AssetRequestEventHandle;
import Brawler.AssetManagement.AssetRequestEventNotifier;

export namespace Brawler
{
	namespace AssetManagement
	{
		class Win32AssetIORequestTracker final : private AssetRequestEventNotifier
		{
		public:
			explicit Win32AssetIORequestTracker(AssetRequestEventHandle&& hAssetRequestEvent);

			Win32AssetIORequestTracker(const Win32AssetIORequestTracker& rhs) = delete;
			Win32AssetIORequestTracker& operator=(const Win32AssetIORequestTracker& rhs) noexcept = delete;

			Win32AssetIORequestTracker(Win32AssetIORequestTracker&& rhs) noexcept = default;
			Win32AssetIORequestTracker& operator=(Win32AssetIORequestTracker&& rhs) noexcept = default;

			void SetActiveRequestCount(const std::uint32_t numActiveRequests);

			void NotifyForAssetIORequestCompletion();
			bool IsAssetRequestEventComplete() const;

		private:
			AssetRequestEventHandle mHAssetRequestEvent;
			std::atomic<std::uint64_t> mActiveRequestCounter;
		};
	}
}