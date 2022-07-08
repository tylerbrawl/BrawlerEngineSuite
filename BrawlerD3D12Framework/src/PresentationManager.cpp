module;
#include <atomic>
#include <array>
#include <cassert>
#include <vector>
#include <mutex>
#include "DxDef.h"

module Brawler.D3D12.PresentationManager;
import Util.General;
import Brawler.D3D12.GPUCommandManager;
import Brawler.JobSystem;

namespace Brawler
{
	namespace D3D12
	{
		void PresentationManager::Initialize()
		{
			mPresentationQueue.Initialize();
		}

		void PresentationManager::EnablePresentationForCurrentFrame()
		{
			// We enable presentation for the current frame by getting the current frame number
			// and then marking the corresponding bool value as true. When the thread which submits
			// commands to the GPU calls PresentationManager::HandleFramePresentation() after it has
			// submitted all of the commands for the current frame, it will find that present must be
			// called for all registered swap chains.
			//
			// This is how we can implement the DirectContext::Present() function as if the function
			// ID3D12GraphicsCommandList::Present() actually existed. (Honestly, that's how it should
			// have been. Instead, we get this awful API where we need to call IDXGISwapChain::Present1()
			// after having created said swap chain with the relevant ID3D12CommandQueue.)

			const std::size_t presentEnabledArrIndex = (Util::Engine::GetCurrentFrameNumber() % mPresentEnabledArr.size());
			mPresentEnabledArr[presentEnabledArrIndex].store(true, std::memory_order::relaxed);
		}

		bool PresentationManager::HandleFramePresentation(const PresentationInfo& presentationInfo)
		{
			// Get the value determining whether or not we need to present this frame. Since GPU commands
			// are not recorded until the FrameGraph reports that any relevant commands from a previous
			// frame have been executed on the GPU timeline, and since we store MAX_FRAMES_IN_FLIGHT values
			// to determine this, we guarantee that we can get this information before it is lost.
			const std::size_t presentEnabledArrIndex = (presentationInfo.FrameNumber % mPresentEnabledArr.size());
			const bool shouldPresentThisFrame = mPresentEnabledArr[presentEnabledArrIndex].exchange(false, std::memory_order::relaxed);

			if (shouldPresentThisFrame) [[likely]]
				SubmitPresentCommands(presentationInfo);

			return shouldPresentThisFrame;
		}

		GPUCommandQueue<GPUCommandQueueType::DIRECT>& PresentationManager::GetPresentationCommandQueue()
		{
			return mPresentationQueue;
		}

		const GPUCommandQueue<GPUCommandQueueType::DIRECT>& PresentationManager::GetPresentationCommandQueue() const
		{
			return mPresentationQueue;
		}

		void PresentationManager::RegisterPresentationCallback(std::move_only_function<HRESULT()>&& callback)
		{
			std::scoped_lock<std::mutex> lock{ mPresentationCallbackArrCritSection };

			mPresentationCallbackArr.push_back(std::move(callback));
		}

		void PresentationManager::ClearPresentationCallbacks()
		{
			std::scoped_lock<std::mutex> lock{ mPresentationCallbackArrCritSection };

			mPresentationCallbackArr.clear();
		}

		void PresentationManager::SubmitPresentCommands(const PresentationInfo& presentationInfo)
		{
			// Have the presentation queue wait on any relevant queues belonging to the GPUCommandManager.
			// Due to the method which the Brawler Engine uses to submit work on the GPU, the most we would
			// only ever have to explicitly wait on would be the DIRECT and COMPUTE queues; this is because
			// the COPY queue is not allowed to be used concurrently with other queues because its set of
			// resource states differs from those of the other queues, even if the resource states share the
			// same name.
			//
			// However, more likely than not, we do not have to wait for the asynchronous compute (COMPUTE)
			// queue to finish execution. There is an associated cost (CPU and GPU) with using fences, so we
			// want to minimize their usage as much as possible. We know that waiting for the DIRECT queue
			// is an absolute and unavoidable necessity, but if we can elide the wait for the COMPUTE queue, then
			// we should.

			assert(presentationInfo.QueuesToSynchronizeWith.ContainsAnyFlag(GPUCommandQueueType::DIRECT) && "ERROR: The presentation queue *MUST* at least synchronize with the DIRECT GPUCommandQueue from the GPUCommandManager! (Otherwise, we cannot guarantee that the back buffers will be in the appropriate resource state!)");
			mPresentationQueue.WaitForGPUCommandQueue(Util::Engine::GetGPUCommandManager().GetGPUCommandQueue<GPUCommandQueueType::DIRECT>());

			if (presentationInfo.QueuesToSynchronizeWith.ContainsAnyFlag(GPUCommandQueueType::COMPUTE)) [[unlikely]]
				mPresentationQueue.WaitForGPUCommandQueue(Util::Engine::GetGPUCommandManager().GetGPUCommandQueue<GPUCommandQueueType::COMPUTE>());

			// As mentioned, we don't need to wait for the COPY queue. We'll assert that this was not specified.
			assert(!presentationInfo.QueuesToSynchronizeWith.ContainsAnyFlag(GPUCommandQueueType::COPY) && "ERROR: The presentation queue should never have to synchronize with the COPY queue! (Did something change with the Brawler Engine's GPU command submission method?)");

			// We can actually call all of the registered callbacks concurrently, but not asynchronously. The
			// reason why asynchronous execution is not possible is because the FrameGraph needs to wait for
			// the Present calls to be finished before it can destroy any transient resources and move on to
			// the next frame; otherwise, we might accidentally destroy resources which are being used by the
			// GPU.
			//
			// The thread calling PresentationManager::SubmitPresentCommands(), however, is the one currently
			// submitting commands to the GPU, so we want to get out of here ASAP. The overhead of submitting
			// CPU jobs to execute presentation callbacks might be too significant to warrant doing so for only
			// a few callbacks. At the same time, however, we expect IDXGISwapChain::Present1() to be a fairly
			// heavy function to execute, since it must interact with the GPU and thus likely consumes driver/kernel
			// time.
			//
			// What follows is a complete heuristic hack based on no real evidence to determine if we should either
			// execute the commands immediately on this thread or create CPU jobs to do this concurrently.
			static constexpr std::size_t MAX_CALLBACKS_FOR_SINGLE_THREAD_EXECUTION = 1;
			bool presentationCallbackFailed = false;

			{
				std::scoped_lock<std::mutex> lock{ mPresentationCallbackArrCritSection };

				if (mPresentationCallbackArr.size() <= MAX_CALLBACKS_FOR_SINGLE_THREAD_EXECUTION)
				{
					for (auto& callback : mPresentationCallbackArr)
					{
						const HRESULT hr = callback();

						// If the presentation failed, then we note that it did fail, but we keep going for the
						// other callbacks.
						if (FAILED(hr)) [[unlikely]]
							presentationCallbackFailed = true;
					}
				}
				else
				{
					// This is a latency-critical task, since we want to call IDXGISwapChain::Present1() as soon as 
					// possible and get this thread back to submitting GPU commands.
					Brawler::JobGroup callbackExecutionGroup{ Brawler::JobPriority::CRITICAL };
					callbackExecutionGroup.Reserve(mPresentationCallbackArr.size());

					std::atomic<bool> sharedCallbackFailureResult{ false };

					for (auto& callback : mPresentationCallbackArr)
					{
						callbackExecutionGroup.AddJob([&callback, &sharedCallbackFailureResult] ()
						{
							const HRESULT hr = callback();

							if (FAILED(hr)) [[unlikely]]
								sharedCallbackFailureResult.store(true, std::memory_order::relaxed);
						});
					}
					
					callbackExecutionGroup.ExecuteJobs();
					presentationCallbackFailed = sharedCallbackFailureResult.load(std::memory_order::relaxed);
				}
			}

			// Now that we have executed every callback, throw an exception if one of the presentation callbacks
			// failed.
			if (presentationCallbackFailed) [[unlikely]]
				throw std::runtime_error{ "ERROR: A presentation callback registered with the PresentationManager failed!" };

			// When this thread returns to the GPUCommandManager, it can now use the GPUCommandQueue belonging to
			// this PresentationManager instance to signal the relevant FrameGraphFenceCollection's DIRECT queue
			// fence. Doing it this way ensures correctness, since the presentation queue always synchronizes with
			// the GPUCommandManager's DIRECT queue; thus, if the presentation queue has finished execution, then
			// the DIRECT queue must have also finished execution.
		}
	}
}