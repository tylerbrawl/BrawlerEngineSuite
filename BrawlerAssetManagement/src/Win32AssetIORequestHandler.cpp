module;
#include <memory>
#include <array>
#include <atomic>
#include <ranges>
#include <cassert>
#include <optional>

module Brawler.AssetManagement.Win32AssetIORequestHandler;
import Brawler.AssetManagement.AssetDependency;
import Brawler.AssetManagement.AssetLoadingMode;
import Brawler.AssetManagement.AssetManager;
import Brawler.JobSystem;

namespace Brawler
{
	namespace AssetManagement
	{
		Win32AssetIORequestHandler::Win32AssetIORequestHandler() :
			mRequestQueueArr(),
			mActiveBuilderArr(),
			mNumThreadsExecutingRequests(0),
			mActiveRequestsExist(false)
		{
			CreateDelayedAssetLoadingJobForCurrentThread();
		}
		
		void Win32AssetIORequestHandler::PrepareAssetIORequest(std::unique_ptr<EnqueuedAssetDependency>&& enqueuedDependency)
		{
			std::unique_ptr<Win32AssetIORequestBuilder> requestBuilderPtr{ std::make_unique<Win32AssetIORequestBuilder>(std::move(enqueuedDependency->HRequestEvent)) };
			enqueuedDependency->Dependency.BuildAssetIORequests(*requestBuilderPtr);

			requestBuilderPtr->Finalize();

			// Exit early if no Win32AssetIORequests were actually made and the asset event was already
			// marked as completed.
			if (requestBuilderPtr->ReadyForDeletion()) [[unlikely]]
				return;

			// Add all of the builder's requests to the queues.
			for (std::underlying_type_t<JobPriority> i = 0; i < std::to_underlying(JobPriority::COUNT); ++i)
			{
				const JobPriority currPriority = static_cast<JobPriority>(i);
				const std::span<Win32AssetIORequest> currRequestSpan{ requestBuilderPtr->GetAssetIORequestSpan(currPriority) };

				for (auto&& request : currRequestSpan)
				{
					// If we fail to push the request to the queue, then we have no choice but to
					// execute it here and now.
					if (!mRequestQueueArr[std::to_underlying(currPriority)].PushBack(std::move(request))) [[unlikely]]
						request.LoadAssetData();
				}
			}

			mActiveBuilderArr.PushBack(std::move(requestBuilderPtr));

			// Use a write-release memory ordering so that the call to
			// Win32AssetIORequestHandler::BeginAssetLoading() "synchronizes with" the store informing
			// a thread that there are new asset I/O requests.
			mActiveRequestsExist.store(true, std::memory_order::release);
		}

		void Win32AssetIORequestHandler::SubmitAssetIORequests()
		{
			// Actual asset I/O requests are handled concurrently by threads when the thread which is
			// told to hold the delayed CPU job realizes that new requests exist. So, we don't actually
			// need to execute any requests here. However, we can clean-up any Win32AssetIORequestBuilder
			// instances which are no longer needed.

			mActiveBuilderArr.EraseIf([] (const std::unique_ptr<Win32AssetIORequestBuilder>& builderPtr)
			{
				return builderPtr->ReadyForDeletion();
			});
		}

		void Win32AssetIORequestHandler::BeginAssetLoading()
		{
			// We actually set this value before we do any asset loading. This is to avoid a race condition.
			// Specifically, let Thread A be the only thread calling Win32AssetIORequestHandler::ExecuteAssetIORequests()
			// at a given point in time. Now, let Thread B be a thread which is submitting asset I/O requests.
			// We need to avoid the following race condition:
			//
			//   - Thread A finds that there are no more requests.
			//   - Thread B adds a new request to the queues and sets mActiveRequestsExist to true.
			//   - Thread A, thinking that there are no requests left, sets mActiveRequestsExist to false. No
			//     thread is then able to execute that request until another asset I/O request is submitted.
			//
			// Doing it this early may lead to false positives (i.e., threads are told to execute asset I/O
			// requests when none exist), but this is considerably better than the alternative (i.e., threads
			// are not informed of new requests).
			mActiveRequestsExist.store(false, std::memory_order::relaxed);

			// Find the highest priority queue with active requests which need to be completed. This will
			// be the priority of the JobGroup which will create the asset loading jobs. There is a race
			// condition here, but it doesn't seem worth adding a std::mutex to fix it, since it isn't
			// going to impact the program's correctness.

			Brawler::JobPriority highestPriority = JobPriority::CRITICAL;
			std::size_t numEmptyQueues = 0;

			for (const auto& requestQueue : mRequestQueueArr | std::views::reverse | std::views::take_while([] (const auto& queue) { return queue.IsEmpty(); }))
			{
				highestPriority = static_cast<JobPriority>(std::to_underlying(highestPriority) - 1);
				++numEmptyQueues;
			}
				
			// If we find that all of the queues are actually empty, then just create the delayed CPU job
			// and exit. We still have a race condition, of course, but this can help mitigate some of the
			// aforementioned false positives, and we'll still always see a new asset I/O request.
			if (numEmptyQueues == mRequestQueueArr.size()) [[unlikely]]
			{
				CreateDelayedAssetLoadingJobForCurrentThread();
				return;
			}

			const std::uint32_t numAssetLoadJobsToCreate = Brawler::AssetManagement::GetSuggestedThreadCountForAssetIORequests(AssetManager::GetInstance().GetAssetLoadingMode());
			std::shared_ptr<std::atomic<std::uint32_t>> remainingThreadsCounter = std::make_shared<std::atomic<std::uint32_t>>(numAssetLoadJobsToCreate);

			Brawler::JobGroup executeAssetLoadGroup{ highestPriority };
			executeAssetLoadGroup.Reserve(numAssetLoadJobsToCreate);

			for (auto i : std::views::iota(0u, numAssetLoadJobsToCreate))
				executeAssetLoadGroup.AddJob([this, remainingThreadsCounter] ()
			{
				ExecuteAssetIORequests(remainingThreadsCounter);
			});

			executeAssetLoadGroup.ExecuteJobsAsync();
		}

		void Win32AssetIORequestHandler::ExecuteAssetIORequests(const std::shared_ptr<std::atomic<std::uint32_t>>& remainingThreadsCounter)
		{
			// Drain the asset I/O request queues in order of decreasing priority.
			for (auto& requestQueue : mRequestQueueArr | std::views::reverse)
			{
				std::optional<Win32AssetIORequest> currRequest{};
				
				do
				{
					currRequest = requestQueue.TryPop();

					if (currRequest.has_value()) [[likely]]
						currRequest->LoadAssetData();
				} while (currRequest.has_value());
			}

			const std::uint32_t numThreadsRemaining = (remainingThreadsCounter->fetch_sub(1, std::memory_order::relaxed) - 1);

			// If this is the last thread to leave, then it gets the delayed CPU job for checking
			// for new asset I/O requests.
			if (numThreadsRemaining == 0)
				CreateDelayedAssetLoadingJobForCurrentThread();
		}

		void Win32AssetIORequestHandler::CreateDelayedAssetLoadingJobForCurrentThread()
		{
			// Create a delayed CPU job which, when its event is signalled, begins asset loading.
			Brawler::DelayedJobGroup delayedAssetLoadGroup{};
			delayedAssetLoadGroup.Reserve(1);

			delayedAssetLoadGroup.AddJob([this] ()
			{
				BeginAssetLoading();
			});

			delayedAssetLoadGroup.SubmitDelayedJobs([this] ()
			{
				return mActiveRequestsExist.load(std::memory_order::acquire);
			});
		}
	}
}