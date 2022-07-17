module;
#include <stdexcept>
#include <filesystem>
#include <format>
#include <array>
#include <memory>
#include <span>
#include <cassert>
#include <ranges>
#include <DxDef.h>

module Brawler.AssetManagement.DirectStorageAssetIORequestHandler;
import Brawler.AssetManagement.BPKArchiveReader;
import Util.General;
import Util.Engine;
import Util.Win32;
import Brawler.AssetManagement.DirectStorageAssetIORequestBuilder;
import Brawler.JobSystem;
import Brawler.Win32EventHandle;
import Brawler.Win32.SafeHandle;
import Brawler.AssetManagement.AssetLoadingMode;
import Brawler.AssetManagement.AssetManager;
import Brawler.ZSTDDecompressionOperation;

namespace
{
	DSTORAGE_CUSTOM_DECOMPRESSION_RESULT HandleDirectStorageDecompressionRequest(const DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST& decompressionRequest)
	{
		assert(decompressionRequest.CompressionFormat == DSTORAGE_COMPRESSION_FORMAT::DSTORAGE_CUSTOM_COMPRESSION_0 && "ERROR: An unrecognized compression format was found in a DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST!");
		
		const std::span<std::byte> destDataSpan{ reinterpret_cast<std::byte*>(decompressionRequest.DstBuffer), decompressionRequest.DstSize };
		const std::span<const std::byte> srcDataSpan{ reinterpret_cast<const std::byte*>(decompressionRequest.SrcBuffer), decompressionRequest.SrcSize };
		Brawler::ZSTDDecompressionOperation decompressOperation{};

		HRESULT hr = decompressOperation.BeginDecompressionOperation(srcDataSpan);

		// Pro Tip: Resources created in UPLOAD heaps are located in write-combined memory, which is incredibly
		// slow to read from. To avoid ZStandard reading from this memory, when the destination buffer is located
		// within an UPLOAD heap, we will instead decompress the data into a temporary array of bytes and
		// copy that into the UPLOAD heap.

		if ((decompressionRequest.Flags & DSTORAGE_CUSTOM_DECOMPRESSION_FLAGS::DSTORAGE_CUSTOM_DECOMPRESSION_FLAG_DEST_IN_UPLOAD_HEAP) != 0)
		{
			// Calling ZSTDDecompressionOperation::FinishDecompressionOperation() without specifying a destination
			// std::span copies the remaining data into a temporary std::vector of bytes for us. This works great
			// for when the destination resource is in an UPLOAD heap.
			Brawler::ZSTDDecompressionOperation::DecompressionResults zstdDecompressResults{ decompressOperation.FinishDecompressionOperation() };

			if (FAILED(zstdDecompressResults.HResult)) [[unlikely]]
				return DSTORAGE_CUSTOM_DECOMPRESSION_RESULT{
					.Id = decompressionRequest.Id,
					.Result = zstdDecompressResults.HResult
				};

			if (decompressionRequest.DstSize < zstdDecompressResults.DecompressedByteArr.size()) [[unlikely]]
				return DSTORAGE_CUSTOM_DECOMPRESSION_RESULT{
					.Id = decompressionRequest.Id,
					.Result = E_NOT_SUFFICIENT_BUFFER
				};

			std::ranges::copy(zstdDecompressResults.DecompressedByteArr, destDataSpan.data());
		}
		else
		{
			// Calling ZSTDDecompressionOperation::FinishDecompressionOperation() with a destination std::span
			// attempts to decompress the remaining data into the specified memory location. When not writing
			// into write-combined memory, this is likely to be the best choice.
			const HRESULT hr = decompressOperation.FinishDecompressionOperation(destDataSpan);

			if (FAILED(hr)) [[unlikely]]
				return DSTORAGE_CUSTOM_DECOMPRESSION_RESULT{
					.Id = decompressionRequest.Id,
					.Result = hr
				};
		}

		return DSTORAGE_CUSTOM_DECOMPRESSION_RESULT{
			.Id = decompressionRequest.Id,
			.Result = S_OK
		};
	}
}

namespace Brawler
{
	namespace AssetManagement
	{
		DirectStorageAssetIORequestHandler::DirectStorageAssetIORequestHandler(Microsoft::WRL::ComPtr<IDStorageFactory>&& directStorageFactory) :
			I_AssetIORequestHandler(),
			mDStorageFactory(std::move(directStorageFactory)),
			mBPKDStorageFile(nullptr),
			mDecompressionEventQueue(nullptr),
			mDirectStorageQueueArr()
		{
			CreateBPKArchiveDirectStorageFile();
			CreateDecompressionEventQueue();
			CreateDirectStorageQueues();

			// On the thread that created the DirectStorageAssetIORequestHandler instance, we create
			// a delayed CPU job using the HANDLE provided by IDStorageCustomDecompressionQueue::GetEvent().
			// This creates a HANDLE for an event which gets signalled by the DirectStorage API when
			// there are pending decompression events.
			//
			// The thread that picks up and executes that CPU job then gets another delayed CPU job
			// for the same thing. Doing it this way allows us to keep at most a single decompression
			// event HANDLE and delayed CPU job in circulation at any given time, avoiding issues
			// such as creating an abundance of delayed CPU jobs and event HANDLEs for the same event.
			Win32::SafeHandle hDirectStorageDecompressionEvent{ mDecompressionEventQueue->GetEvent() };
			assert(Util::Win32::IsHandleValid(hDirectStorageDecompressionEvent.get()) && "ERROR: DirectStorage returned an invalid HANDLE for a decompression event after calling IDStorageCustomDecompressionQueue::GetEvent()!");

			Brawler::DelayedJobGroup delayedDecompressionGroup{};
			delayedDecompressionGroup.Reserve(1);

			delayedDecompressionGroup.AddJob([this] ()
			{
				BeginCPUDecompression();
			});

			delayedDecompressionGroup.SubmitDelayedJobs(Win32EventHandle{ std::move(hDirectStorageDecompressionEvent) });
		}

		void DirectStorageAssetIORequestHandler::PrepareAssetIORequest(EnqueuedAssetDependency&& enqueuedDependency)
		{
			// Let the asset dependency resolver callbacks in the AssetDependency tell us which assets need to
			// be loaded.
			DirectStorageAssetIORequestBuilder requestBuilder{ *(mDStorageFactory.Get()), *(mBPKDStorageFile.Get())};
			enqueuedDependency.Dependency.BuildAssetIORequests(requestBuilder);

			std::unique_ptr<PendingDirectStorageRequest> pendingRequestPtr{ std::make_unique<PendingDirectStorageRequest>(std::move(requestBuilder), std::move(enqueuedDependency.HRequestEvent)) };

			for (auto& queue : mDirectStorageQueueArr)
				queue.EnqueueRequest(*pendingRequestPtr);

			mPendingRequestArr.PushBack(std::move(pendingRequestPtr));
		}

		void DirectStorageAssetIORequestHandler::SubmitAssetIORequests()
		{
			// After we have created all of the DirectStorageAssetIORequestBuilder instances for
			// this iteration of asset I/O request handling, we call IDStorageQueue::Submit() on each
			// queue to have DirectStorage begin processing requests.

			for (auto& queue : mDirectStorageQueueArr)
				queue.Submit();

			ClearCompletedRequests();
		}

		IDStorageFactory& DirectStorageAssetIORequestHandler::GetDirectStorageFactory()
		{
			assert(mDStorageFactory != nullptr);
			return *(mDStorageFactory.Get());
		}

		const IDStorageFactory& DirectStorageAssetIORequestHandler::GetDirectStorageFactory() const
		{
			assert(mDStorageFactory != nullptr);
			return *(mDStorageFactory.Get());
		}

		void DirectStorageAssetIORequestHandler::CreateBPKArchiveDirectStorageFile()
		{
			const std::filesystem::path& bpkArchivePath{ BPKArchiveReader::GetBPKArchiveFilePath() };

			std::error_code errorCode{};
			const bool bpkArchiveExists = std::filesystem::exists(bpkArchivePath, errorCode);

			if (errorCode) [[unlikely]]
				throw std::runtime_error{ std::format("ERROR: The attempt to check if the BPK archive exists for DirectStorage usage failed with the following error: {}", errorCode.message()) };

			if (!bpkArchiveExists) [[unlikely]]
				throw std::runtime_error{ "ERROR: The BPK data archive could not be found!" };

			Util::General::CheckHRESULT(mDStorageFactory->OpenFile(bpkArchivePath.c_str(), IID_PPV_ARGS(&mBPKDStorageFile)));
		}

		void DirectStorageAssetIORequestHandler::CreateDecompressionEventQueue()
		{
			// For some reason, we have to get the decompression queue by using QueryInterface()
			// on the IDStorageFactory. Don't ask me why.
			Util::General::CheckHRESULT(mDStorageFactory.As(&mDecompressionEventQueue));
		}

		void DirectStorageAssetIORequestHandler::CreateDirectStorageQueues()
		{
			for (auto priorityNum : std::views::iota(std::to_underlying(Brawler::JobPriority::LOW), std::to_underlying(Brawler::JobPriority::COUNT)))
				mDirectStorageQueueArr[priorityNum] = DirectStorageQueue{ *this, static_cast<Brawler::JobPriority>(priorityNum) };
		}

		void DirectStorageAssetIORequestHandler::ClearCompletedRequests()
		{
			mPendingRequestArr.EraseIf([] (const std::unique_ptr<PendingDirectStorageRequest>& requestPtr) { return requestPtr->ReadyForDeletion(); });
		}

		void DirectStorageAssetIORequestHandler::BeginCPUDecompression() const
		{
			// The DirectStorage API does not give us a way to query for the number of pending CPU decompression
			// requests, and even if it did, the result would always inherently be racy. So, what we will do
			// instead is have the threads performing the decompression work pull requests one at a time from
			// the queue.
			//
			// Unfortunately, the API does not tell us what the priority of the queue which the corresponding
			// DSTORAGE_REQUEST instance for a given decompression request was sent to, so a normal job priority
			// is as good of a guess as any.

			const std::uint32_t numJobsToCreate = Brawler::AssetManagement::GetSuggestedThreadCountForAssetIORequests(AssetManager::GetInstance().GetAssetLoadingMode());

			Brawler::JobGroup decompressionJobGroup{};
			decompressionJobGroup.Reserve(numJobsToCreate);

			// We use a shared_ptr here because it must be shared across threads.
			const std::shared_ptr<std::atomic<std::uint32_t>> remainingThreadsCounter{ std::make_shared<std::atomic<std::uint32_t>>(numJobsToCreate) };

			for (auto i : std::views::iota(0u, numJobsToCreate))
				decompressionJobGroup.AddJob([this, remainingThreadsCounter] ()
			{
				ExecuteCPUDecompressionTasks(remainingThreadsCounter);
			});

			decompressionJobGroup.ExecuteJobs();
		}

		void DirectStorageAssetIORequestHandler::ExecuteCPUDecompressionTasks(const std::shared_ptr<std::atomic<std::uint32_t>>& remainingThreadsCounter) const
		{
			while (true)
			{
				std::uint32_t numRequestsReceived = 0;
				DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST decompressionRequest{};

				Util::General::CheckHRESULT(mDecompressionEventQueue->GetRequests(1, &decompressionRequest, &numRequestsReceived));

				if (numRequestsReceived == 0) [[unlikely]]
					break;

				DSTORAGE_CUSTOM_DECOMPRESSION_RESULT decompressionResult{ HandleDirectStorageDecompressionRequest(decompressionRequest) };
				Util::General::CheckHRESULT(mDecompressionEventQueue->SetRequestResults(1, &decompressionResult));
			}

			const std::uint32_t numThreadsRemaining = (remainingThreadsCounter->fetch_sub(1, std::memory_order::relaxed) - 1);

			// The last thread to leave this function is given the HANDLE for the next set of DirectStorage
			// decompression requests. This HANDLE is used to create a delayed CPU job which, once the event
			// is signalled, will allow the decompression process to start again.
			if (numThreadsRemaining == 0)
			{
				Brawler::DelayedJobGroup delayedDecompressionGroup{};
				delayedDecompressionGroup.Reserve(1);

				delayedDecompressionGroup.AddJob([this] ()
				{
					BeginCPUDecompression();
				});

				Win32::SafeHandle hDecompressionEvent{ mDecompressionEventQueue->GetEvent() };
				assert(Util::Win32::IsHandleValid(hDecompressionEvent.get()) && "ERROR: DirectStorage returned an invalid HANDLE for a decompression event after calling IDStorageCustomDecompressionQueue::GetEvent()!");

				delayedDecompressionGroup.SubmitDelayedJobs(Win32EventHandle{ std::move(hDecompressionEvent) });
			}
		}
	}
}