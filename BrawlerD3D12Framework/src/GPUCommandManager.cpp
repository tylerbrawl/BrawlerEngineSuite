module;
#include <memory>
#include <span>
#include <array>
#include <atomic>
#include <mutex>
#include <queue>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.GPUCommandManager;
import Brawler.JobSystem;
import Brawler.D3D12.FrameGraphFenceCollection;
import Brawler.D3D12.PresentationManager;

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void GPUCommandManager::HaltGPUCommandQueueForPreviousSubmission() const
		{
			if (QueueType != GPUCommandQueueType::DIRECT && (mLastSubmissionQueues & GPUCommandQueueType::DIRECT).CountOneBits() != 0)
				GetGPUCommandQueue<QueueType>().WaitForGPUCommandQueue(mDirectCmdQueue);

			if (QueueType != GPUCommandQueueType::COMPUTE && (mLastSubmissionQueues & GPUCommandQueueType::COMPUTE).CountOneBits() != 0)
				GetGPUCommandQueue<QueueType>().WaitForGPUCommandQueue(mComputeCmdQueue);

			if (QueueType != GPUCommandQueueType::COPY && (mLastSubmissionQueues & GPUCommandQueueType::COPY).CountOneBits() != 0)
				GetGPUCommandQueue<QueueType>().WaitForGPUCommandQueue(mCopyCmdQueue);
		}

		void GPUCommandManager::Initialize()
		{
			mCmdContextVaultPtr = std::make_unique<GPUCommandContextVault>();
			
			mDirectCmdQueue.Initialize();
			mComputeCmdQueue.Initialize();
			mCopyCmdQueue.Initialize();
		}

		std::span<GPUCommandContextSubmissionPoint> GPUCommandManager::BeginGPUCommandContextSubmission(FrameGraphFenceCollection& fenceCollection, const std::size_t numExecutionModules)
		{
			std::span<GPUCommandContextSubmissionPoint> submitPointSpan{};
			std::atomic<bool> spanInitialized{ false };

			Brawler::JobGroup gpuSubmitJobGroup{};
			gpuSubmitJobGroup.Reserve(1);

			struct GPUSubmitJobInfo
			{
				FrameGraphFenceCollection& FenceCollection;
				std::size_t NumExecutionModules;
				std::span<GPUCommandContextSubmissionPoint>& SubmitPointSpan;
				std::atomic<bool>& SpanInitialized;
				std::uint64_t FrameNumber;
			};

			const GPUSubmitJobInfo submitJobInfo{
				.FenceCollection{fenceCollection},
				.NumExecutionModules = numExecutionModules,
				.SubmitPointSpan{submitPointSpan},
				.SpanInitialized{spanInitialized},
				.FrameNumber = Util::Engine::GetCurrentFrameNumber()
			};

			gpuSubmitJobGroup.AddJob([this, &submitJobInfo] ()
			{
				const std::uint64_t frameNumber = submitJobInfo.FrameNumber;
				const std::size_t sinkIndex = (frameNumber % mCmdContextSinkArr.size());

				submitJobInfo.SubmitPointSpan = mCmdContextSinkArr[sinkIndex].InitializeSinkForCurrentFrame(submitJobInfo.NumExecutionModules);

				bool shouldSubmitGPUJobs = false;
				FrameGraphSubmissionInfo submissionInfo{
					.FenceCollectionPtr = &(submitJobInfo.FenceCollection),
					.FrameNumber = submitJobInfo.FrameNumber
				};

				{
					std::scoped_lock<std::mutex> lock{ mSinkInfo.CritSection };

					mSinkInfo.SubmissionInfoQueue.push(std::move(submissionInfo));
					assert(mSinkInfo.SubmissionInfoQueue.size() <= mCmdContextSinkArr.size());

					shouldSubmitGPUJobs = !(mSinkInfo.IsThreadHandlingSinks);
					mSinkInfo.IsThreadHandlingSinks = true;
				}

				submitJobInfo.SpanInitialized.store(true, std::memory_order::release);
				submitJobInfo.SpanInitialized.notify_all();

				if (shouldSubmitGPUJobs)
					DrainGPUCommandContextSinks(sinkIndex);
			});

			gpuSubmitJobGroup.ExecuteJobsAsync();
			spanInitialized.wait(false, std::memory_order::acquire);

			return submitPointSpan;
		}

		void GPUCommandManager::SubmitGPUCommandContextGroup(GPUCommandContextGroup&& cmdContextGroup)
		{
			// First, we need to tell the appropriate queues to wait on the queues from the previous
			// submission. In doing this, we guarantee the order of execution that the API specifies.
			const Brawler::CompositeEnum<GPUCommandQueueType> usedQueues{ cmdContextGroup.GetUsedQueues() };

			if (usedQueues.CountOneBits() != 0) [[likely]]
			{
				if ((usedQueues & GPUCommandQueueType::DIRECT).CountOneBits() != 0)
					HaltGPUCommandQueueForPreviousSubmission<GPUCommandQueueType::DIRECT>();

				if ((usedQueues & GPUCommandQueueType::COMPUTE).CountOneBits() != 0)
					HaltGPUCommandQueueForPreviousSubmission<GPUCommandQueueType::COMPUTE>();

				if ((usedQueues & GPUCommandQueueType::COPY).CountOneBits() != 0)
					HaltGPUCommandQueueForPreviousSubmission<GPUCommandQueueType::COPY>();

				mLastSubmissionQueues = usedQueues;

				// Next, have the command queues submit the command lists belonging to the
				// GPUCommandContext instances to the GPU. The GPUCommandQueues will also set the required
				// fence for the GPUCommandContext instances, informing them of when they can be used again.
				mDirectCmdQueue.SubmitGPUCommandContextGroup(cmdContextGroup);
				mComputeCmdQueue.SubmitGPUCommandContextGroup(cmdContextGroup);
				mCopyCmdQueue.SubmitGPUCommandContextGroup(cmdContextGroup);
			}

			// Finally, we return the used GPUCommandContext instances back to the
			// GPUCommandContextVault, so that they may be re-used later.
			const auto returnCmdContextsLambda = [this]<GPUCommandQueueType QueueType>(GPUCommandContextGroup& cmdContextGroup)
			{
				for (auto&& cmdContext : cmdContextGroup.GetGPUCommandContexts<QueueType>())
					mCmdContextVaultPtr->ReturnCommandContext<QueueType>(std::move(cmdContext));
			};

			returnCmdContextsLambda.operator()<GPUCommandQueueType::DIRECT>(cmdContextGroup);
			returnCmdContextsLambda.operator()<GPUCommandQueueType::COMPUTE>(cmdContextGroup);
			returnCmdContextsLambda.operator()<GPUCommandQueueType::COPY>(cmdContextGroup);
		}

		GPUCommandContextVault& GPUCommandManager::GetGPUCommandContextVault()
		{
			assert(mCmdContextVaultPtr != nullptr);
			return *mCmdContextVaultPtr;
		}

		void GPUCommandManager::DrainGPUCommandContextSinks(std::size_t beginSinkIndex)
		{
			bool keepGoing = true;
			
			while (keepGoing)
			{
				const FrameGraphSubmissionInfo* submissionInfoPtr = nullptr;

				{
					std::scoped_lock<std::mutex> lock{ mSinkInfo.CritSection };

					assert(!mSinkInfo.SubmissionInfoQueue.empty());
					submissionInfoPtr = &(mSinkInfo.SubmissionInfoQueue.front());
				}

				assert(submissionInfoPtr != nullptr);
				EnsureGPUResidencyForCurrentFrame(*(submissionInfoPtr->FenceCollectionPtr));
				
				mCmdContextSinkArr[beginSinkIndex].RunGPUSubmissionLoop();
				beginSinkIndex = ((beginSinkIndex + 1) % mCmdContextSinkArr.size());

				FrameGraphFenceCollection* const fenceCollectionPtr = submissionInfoPtr->FenceCollectionPtr;
				const std::uint64_t submittedFrameNumber = submissionInfoPtr->FrameNumber;

				// We need to call PresentationManager::HandleFramePresentation() outside of the lock.
				// Otherwise, we can get a deadlock, because this function might launch CPU jobs and potentially
				// cause this or another thread to attempt to acquire the mSinkInfo.CritSection lock.
				//
				// However, this should still be safe to do.
				//
				//   - We already guarantee that at most one thread is in GPUCommandManager::DrainGPUCommandContextSinks()
				//     at any point in time.
				//
				//   - We know that commands for the frame MAX_FRAMES_IN_FLIGHT frames away from submittedFrameNumber
				//     will not begin recording until all of the FrameGraphFenceCollection's fences have been signalled.
				//     We wait to do this until after presentation.
				bool presentationOccurred = false;
				try
				{
					presentationOccurred = Util::Engine::GetPresentationManager().HandleFramePresentation(PresentationManager::PresentationInfo{
						.FrameNumber = submittedFrameNumber,
						.QueuesToSynchronizeWith = mLastSubmissionQueues
					});
				}
				catch (...)
				{
					// If PresentationManager::HandleFramePresentation() throws an exception because a callback failed,
					// then we're probably screwed, anyways. However, if this is the case, we should still ensure that
					// mSinkInfo.IsThreadHandlingSinks is set to false, just in case we can, in fact, recover.

					{
						std::scoped_lock<std::mutex> lock{ mSinkInfo.CritSection };
						
						mSinkInfo.IsThreadHandlingSinks = false;
					}

					std::rethrow_exception(std::current_exception());
				}

				{
					std::scoped_lock<std::mutex> lock{ mSinkInfo.CritSection };
					
					mSinkInfo.SubmissionInfoQueue.pop();
					submissionInfoPtr = nullptr;

					if (mSinkInfo.SubmissionInfoQueue.empty())
					{
						mSinkInfo.IsThreadHandlingSinks = false;
						keepGoing = false;
					}

					// There is a race condition with signalling the FrameGraphFenceCollections
					// outside of the std::mutex. Specifically, it would be possible for commands
					// for a FrameGraph B to be submitted to the queues before the
					// ID3D12CommandQueue::Signal() function can be called on FrameGraph A's fences.
					//
					// This would result in FrameGraph A needing to wait for some of FrameGraph B's
					// commands to finish before it could conclude that all of its commands have
					// been executed on the GPU.
					assert(fenceCollectionPtr != nullptr);
					
					// If we presented for this frame, then wait for the presentation queue belonging to the
					// PresentationManager. Otherwise, wait for the DIRECT queue of this GPUCommandManager instance.
					// This works because the presentation queue always synchronizes with the DIRECT command queue,
					// so either way, we guarantee that all of the relevant commands have finished.
					if (presentationOccurred) [[likely]]
						fenceCollectionPtr->SignalFence<FrameGraphFenceCollection::FenceIndex::DIRECT_QUEUE>(Util::Engine::GetPresentationManager().GetPresentationCommandQueue().GetD3D12CommandQueue());
					else [[unlikely]]
						fenceCollectionPtr->SignalFence<FrameGraphFenceCollection::FenceIndex::DIRECT_QUEUE>(mDirectCmdQueue.GetD3D12CommandQueue());

					fenceCollectionPtr->SignalFence<FrameGraphFenceCollection::FenceIndex::COMPUTE_QUEUE>(mComputeCmdQueue.GetD3D12CommandQueue());
					fenceCollectionPtr->SignalFence<FrameGraphFenceCollection::FenceIndex::COPY_QUEUE>(mCopyCmdQueue.GetD3D12CommandQueue());
				}
			}
		}

		void GPUCommandManager::EnsureGPUResidencyForCurrentFrame(FrameGraphFenceCollection& fenceCollection) const
		{
			fenceCollection.EnsureGPUResidencyForCommandQueue(mDirectCmdQueue.GetD3D12CommandQueue());
			fenceCollection.EnsureGPUResidencyForCommandQueue(mComputeCmdQueue.GetD3D12CommandQueue());
			fenceCollection.EnsureGPUResidencyForCommandQueue(mCopyCmdQueue.GetD3D12CommandQueue());
		}
	}
}