module;
#include <atomic>
#include <span>
#include <DxDef.h>

export module Brawler.AssetManagement.PendingDirectStorageRequest;
import Brawler.AssetManagement.AssetRequestEventHandle;
import Brawler.AssetManagement.DirectStorageAssetIORequestBuilder;
import Brawler.JobPriority;
import Brawler.AssetManagement.AssetRequestEventNotifier;

export namespace Brawler
{
	namespace AssetManagement
	{
		struct DirectStorageStatusArrayEntry;
	}
}

export namespace Brawler
{
	namespace AssetManagement
	{
		class PendingDirectStorageRequest final : private AssetRequestEventNotifier
		{
		public:
			PendingDirectStorageRequest() = default;
			PendingDirectStorageRequest(DirectStorageAssetIORequestBuilder&& requestBuilder, AssetRequestEventHandle&& hAssetRequestEvent);

			PendingDirectStorageRequest(const PendingDirectStorageRequest& rhs) = delete;
			PendingDirectStorageRequest& operator=(const PendingDirectStorageRequest& rhs) = delete;

			PendingDirectStorageRequest(PendingDirectStorageRequest&& rhs) noexcept = default;
			PendingDirectStorageRequest& operator=(PendingDirectStorageRequest&& rhs) noexcept = default;

			std::span<const DSTORAGE_REQUEST> GetDStorageRequestSpan(const Brawler::JobPriority priority) const;

			void CreateRequestFinishedNotification(const DirectStorageStatusArrayEntry& statusArrayEntry);
			bool ReadyForDeletion() const;

		private:
			DirectStorageAssetIORequestBuilder mRequestBuilder;
			AssetRequestEventHandle mHAssetRequestEvent;
			std::atomic<std::uint32_t> mNumStatusArrayEntriesCompleted;
			std::uint32_t mTotalStatusArrayEntryCount;
		};
	}
}