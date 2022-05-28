module;
#include <atomic>
#include <span>
#include <cassert>
#include <DxDef.h>

module Brawler.AssetManagement.PendingDirectStorageRequest;
import Brawler.DelayedJobGroup;
import Brawler.AssetManagement.DirectStorageStatusArrayEntry;
import Util.General;

namespace Brawler
{
	namespace AssetManagement
	{
		PendingDirectStorageRequest::PendingDirectStorageRequest(DirectStorageAssetIORequestBuilder&& requestBuilder, AssetRequestEventHandle&& hAssetRequestEvent) :
			AssetRequestEventNotifier(),
			mRequestBuilder(std::move(requestBuilder)),
			mHAssetRequestEvent(std::move(hAssetRequestEvent)),
			mNumStatusArrayEntriesCompleted(0),
			mTotalStatusArrayEntryCount(0)
		{
			// We will have one IDStorageStatusArray entry for each priority level.
			for (std::underlying_type_t<Brawler::JobPriority> i = 0; i < std::to_underlying(Brawler::JobPriority::COUNT); ++i)
			{
				const std::span<const DSTORAGE_REQUEST> requestSpanForPriorityLevel{ mRequestBuilder.GetDStorageRequestSpan(static_cast<Brawler::JobPriority>(i)) };

				if (!requestSpanForPriorityLevel.empty())
					++mTotalStatusArrayEntryCount;
			}
		}

		std::span<const DSTORAGE_REQUEST> PendingDirectStorageRequest::GetDStorageRequestSpan(const Brawler::JobPriority priority) const
		{
			return mRequestBuilder.GetDStorageRequestSpan(priority);
		}

		void PendingDirectStorageRequest::CreateRequestFinishedNotification(const DirectStorageStatusArrayEntry& statusArrayEntry)
		{
			Brawler::DelayedJobGroup requestFinishedNotificationJob{ statusArrayEntry.QueuePriority };
			requestFinishedNotificationJob.Reserve(1);

			// Eventually, we want to update this PendingDirectStorageRequest instance to account for a finished
			// request.
			requestFinishedNotificationJob.AddJob([this] ()
			{
				const std::uint32_t numEntriesCompleted = (mNumStatusArrayEntriesCompleted.fetch_add(1, std::memory_order::relaxed) + 1);

				if (numEntriesCompleted == mTotalStatusArrayEntryCount)
					AssetRequestEventNotifier::MarkAssetRequestAsCompleted(mHAssetRequestEvent);
			});

			// We want to do this after the corresponding IDStorageStatusArray tells us that the request is
			// complete. (Obviously, we capture things by value to ensure that the thread does not later access
			// garbage. Also, here's a "fun" fact: C# has no explicit copy by value mechanism for lambda functions.
			// Isn't that just *wonderful*?)
			requestFinishedNotificationJob.SubmitDelayedJobs([statusArrayPtr = std::addressof(statusArrayEntry.StatusArray), entryIndex = statusArrayEntry.StatusArrayEntryIndex]()
			{
				const HRESULT hr = statusArrayPtr->GetHResult(entryIndex);

				switch (hr)
				{
				case S_OK:
					return true;

				case E_PENDING:
					return false;

				default: [[unlikely]]
				{
					Util::General::CheckHRESULT(hr);

					assert(false);
					std::unreachable();

					return false;
				}
				}
			});
		}

		bool PendingDirectStorageRequest::ReadyForDeletion() const
		{
			return mHAssetRequestEvent.IsAssetRequestComplete();
		}
	}
}