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

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		const GPUCommandQueue<QueueType>& GPUCommandManager::GetGPUCommandQueue() const
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return mDirectCmdQueue;

			else if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return mComputeCmdQueue;

			else
				return mCopyCmdQueue;
		}
		
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
				const std::size_t sinkIndex = (submitJobInfo.FrameNumber % mCmdContextSinkArr.size());

				submitJobInfo.SubmitPointSpan = mCmdContextSinkArr[sinkIndex].InitializeSinkForCurrentFrame(submitJobInfo.NumExecutionModules);

				// Create a copy of the FrameGraphFenceCollection* before allowing the other thread
				// to destroy the GPUSubmitJobInfo.
				FrameGraphFenceCollection* const fenceCollectionPtr = &(submitJobInfo.FenceCollection);

				submitJobInfo.SpanInitialized.store(true);
				submitJobInfo.SpanInitialized.notify_all();

				bool shouldSubmitGPUJobs = false;

				{
					std::scoped_lock<std::mutex> lock{ mSinkInfo.CritSection };

					mSinkInfo.FenceCollectionQueue.push(fenceCollectionPtr);
					assert(mSinkInfo.FenceCollectionQueue.size() <= mCmdContextSinkArr.size());

					shouldSubmitGPUJobs = !(mSinkInfo.IsThreadHandlingSinks);
					mSinkInfo.IsThreadHandlingSinks = true;
				}

				if (shouldSubmitGPUJobs)
					DrainGPUCommandContextSinks(sinkIndex);
			});

			gpuSubmitJobGroup.ExecuteJobsAsync();
			spanInitialized.wait(false);

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
				FrameGraphFenceCollection* fenceCollectionPtr = nullptr;

				{
					std::scoped_lock<std::mutex> lock{ mSinkInfo.CritSection };

					assert(!mSinkInfo.FenceCollectionQueue.empty());
					fenceCollectionPtr = mSinkInfo.FenceCollectionQueue.front();
				}

				assert(fenceCollectionPtr != nullptr);
				EnsureGPUResidencyForCurrentFrame(*fenceCollectionPtr);
				
				mCmdContextSinkArr[beginSinkIndex].RunGPUSubmissionLoop();
				beginSinkIndex = ((beginSinkIndex + 1) % mCmdContextSinkArr.size());

				{
					std::scoped_lock<std::mutex> lock{ mSinkInfo.CritSection };
					
					mSinkInfo.FenceCollectionQueue.pop();

					if (mSinkInfo.FenceCollectionQueue.empty())
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