module;
#include <cassert>
#include <array>
#include <string_view>
#include <mutex>
#include <span>
#include <DxDef.h>

module Brawler.AssetManagement.DirectStorageQueue;
import Util.General;
import Util.DirectStorage;
import Util.Engine;
import Brawler.AssetManagement.DirectStorageAssetIORequestHandler;
import Brawler.AssetManagement.PendingDirectStorageRequest;
import Brawler.AssetManagement.DirectStorageStatusArrayEntry;

namespace
{
	consteval std::array<DSTORAGE_QUEUE_DESC, std::to_underlying(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT)> CreateDefaultDirectStorageQueueDescArray()
	{
		const DSTORAGE_QUEUE_DESC defaultQueueDesc{
			.SourceType = DSTORAGE_REQUEST_SOURCE_TYPE::DSTORAGE_REQUEST_SOURCE_FILE,
			.Capacity = Util::DirectStorage::DIRECT_STORAGE_QUEUE_CAPACITY,
			.Priority{},
			.Name{ nullptr },
			.Device{ nullptr }
		};
		std::array<DSTORAGE_QUEUE_DESC, std::to_underlying(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT)> queueDescArr{
			defaultQueueDesc,
			defaultQueueDesc,
			defaultQueueDesc,
			defaultQueueDesc
		};

		DSTORAGE_PRIORITY currPriority = DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_FIRST;
		for (auto& queueDesc : queueDescArr)
		{
			queueDesc.Priority = currPriority;
			currPriority = static_cast<DSTORAGE_PRIORITY>(std::to_underlying(currPriority) + 1);
		}

		if constexpr (Util::General::GetBuildMode() != Util::General::BuildMode::RELEASE)
		{
			queueDescArr[static_cast<std::size_t>(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_LOW) + 1].Name = "DirectStorage Queue - Low Priority";
			queueDescArr[static_cast<std::size_t>(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_NORMAL) + 1].Name = "DirectStorage Queue - Normal Priority";
			queueDescArr[static_cast<std::size_t>(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_HIGH) + 1].Name = "DirectStorage Queue - High Priority";
			queueDescArr[static_cast<std::size_t>(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_REALTIME) + 1].Name = "DirectStorage Queue - Real-Time Priority";
		}

		return queueDescArr;
	}

	consteval std::array<std::string_view, std::to_underlying(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT)> CreateDirectStorageStatusArrayNameArray()
	{
		if constexpr (Util::General::GetBuildMode() != Util::General::BuildMode::RELEASE)
		{
			return std::array<std::string_view, std::to_underlying(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT)>{
				std::string_view{ "DirectStorage Status Array - Low Priority Queue" },
				std::string_view{ "DirectStorage Status Array - Normal Priority Queue" },
				std::string_view{ "DirectStorage Status Array - High Priority Queue" },
				std::string_view{ "DirectStorage Status Array - Real-Time Priority Queue" }
			};
		}
		else
		{
			return std::array<std::string_view, std::to_underlying(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT)>{
				std::string_view{},
				std::string_view{},
				std::string_view{},
				std::string_view{}
			};
		}
	}
}



namespace Brawler
{
	namespace AssetManagement
	{
		DirectStorageQueue::DirectStorageQueue(DirectStorageAssetIORequestHandler& owningRequestHandler, const Brawler::JobPriority queuePriority) :
			mOwningRequestHandlerPtr(std::addressof(owningRequestHandler)),
			mDStorageStatusArray(nullptr),
			mDStorageQueue(nullptr),
			mQueuePriority(queuePriority),
			mCurrStatusArrayEntryIndex(0),
			mSubmissionCritSection()
		{
			CreateDStorageQueue();
			CreateDStorageStatusArray();
		}

		DirectStorageQueue::DirectStorageQueue(DirectStorageQueue&& rhs) noexcept :
			mOwningRequestHandlerPtr(),
			mDStorageStatusArray(),
			mDStorageQueue(),
			mQueuePriority(),
			mCurrStatusArrayEntryIndex(),
			mSubmissionCritSection()
		{
			std::scoped_lock<std::mutex> lock{ rhs.mSubmissionCritSection };

			mOwningRequestHandlerPtr = rhs.mOwningRequestHandlerPtr;
			rhs.mOwningRequestHandlerPtr = nullptr;

			mDStorageStatusArray = std::move(rhs.mDStorageStatusArray);

			mDStorageQueue = std::move(rhs.mDStorageQueue);

			mCurrStatusArrayEntryIndex = rhs.mCurrStatusArrayEntryIndex;

			mQueuePriority = rhs.mQueuePriority;
		}

		DirectStorageQueue& DirectStorageQueue::operator=(DirectStorageQueue&& rhs) noexcept
		{
			std::scoped_lock<std::mutex, std::mutex> lock{ mSubmissionCritSection, rhs.mSubmissionCritSection };

			mOwningRequestHandlerPtr = rhs.mOwningRequestHandlerPtr;
			rhs.mOwningRequestHandlerPtr = nullptr;

			mDStorageStatusArray = std::move(rhs.mDStorageStatusArray);

			mDStorageQueue = std::move(rhs.mDStorageQueue);

			mCurrStatusArrayEntryIndex = rhs.mCurrStatusArrayEntryIndex;

			mQueuePriority = rhs.mQueuePriority;

			return *this;
		}

		void DirectStorageQueue::EnqueueRequest(PendingDirectStorageRequest& request)
		{
			// Ignore this request if it has nothing to submit to this queue.
			const std::span<const DSTORAGE_REQUEST> relevantRequestSpan{ request.GetDStorageRequestSpan(mQueuePriority) };

			// We add the [[likely]] attribute because most of the time, a single AssetDependency
			// instance is likely to register all of its requests under a single priority. So, for
			// most of the queues, an AssetDependency's requests will be ignored.
			if (relevantRequestSpan.empty()) [[likely]]
				return;

			// The DirectStorage API does not make it easy to submit requests in a multi-threaded
			// fashion, since we have to enqueue each request separately in a single function call.
			// So, we have to use a std::mutex here.
			//
			// I was thinking of letting the threads add PendingDirectStorageRequest instances to
			// a Brawler::ThreadSafeQueue concurrently, and then having a single thread make all
			// of the DirectStorage API calls. However, I'm not sure if this is really the best
			// way to go about it. We can always benchmark it later. I'm just putting this here in
			// case the current implementation does turn out to be slow.

			std::uint32_t requestStatusArrayIndex = 0;

			{
				std::scoped_lock<std::mutex> lock{ mSubmissionCritSection };

				for (const auto& request : relevantRequestSpan)
					mDStorageQueue->EnqueueRequest(&request);

				requestStatusArrayIndex = mCurrStatusArrayEntryIndex;
				mCurrStatusArrayEntryIndex = ((mCurrStatusArrayEntryIndex + 1) % Util::DirectStorage::DIRECT_STORAGE_QUEUE_CAPACITY);

				mDStorageQueue->EnqueueStatus(mDStorageStatusArray.Get(), requestStatusArrayIndex);
			}
			
			request.CreateRequestFinishedNotification(DirectStorageStatusArrayEntry{
				.StatusArray{*(mDStorageStatusArray.Get())},
				.StatusArrayEntryIndex = requestStatusArrayIndex,
				.QueuePriority = mQueuePriority
			});
		}

		void DirectStorageQueue::Submit() const
		{
			// The documentation for DirectStorage doesn't really say anything about thread safety.
			// I find it hard to believe that they wouldn't make an API like this thread safe, though.
			//
			// Anyways, we still need to take the lock here to ensure correctness, although this function
			// shouldn't ever be called while another thread is calling DirectStorageQueue::EnqueueRequest().

			std::scoped_lock<std::mutex> lock{ mSubmissionCritSection };

			mDStorageQueue->Submit();
		}

		void DirectStorageQueue::CreateDStorageQueue()
		{
			static constexpr std::array<DSTORAGE_QUEUE_DESC, std::to_underlying(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT)> DEFAULT_QUEUE_DESC_ARR{ CreateDefaultDirectStorageQueueDescArray() };

			DSTORAGE_QUEUE_DESC queueDesc{ DEFAULT_QUEUE_DESC_ARR[std::to_underlying(mQueuePriority)] };
			queueDesc.Device = &(Util::Engine::GetD3D12Device());

			Util::General::CheckHRESULT(mOwningRequestHandlerPtr->GetDirectStorageFactory().CreateQueue(&queueDesc, IID_PPV_ARGS(&mDStorageQueue)));
		}

		void DirectStorageQueue::CreateDStorageStatusArray()
		{
			static constexpr std::array<std::string_view, std::to_underlying(DSTORAGE_PRIORITY::DSTORAGE_PRIORITY_COUNT)> STATUS_ARRAY_NAME_ARR{ CreateDirectStorageStatusArrayNameArray() };

			Util::General::CheckHRESULT(mOwningRequestHandlerPtr->GetDirectStorageFactory().CreateStatusArray(Util::DirectStorage::DIRECT_STORAGE_QUEUE_CAPACITY, STATUS_ARRAY_NAME_ARR[std::to_underlying(mQueuePriority)].data(), IID_PPV_ARGS(&mDStorageStatusArray)));
		}
	}
}