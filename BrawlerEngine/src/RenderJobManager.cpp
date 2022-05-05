module;
#include <atomic>
#include <memory>
#include <vector>
#include <optional>
#include <deque>
#include <forward_list>
#include <thread>
#include "DxDef.h"

module Brawler.RenderJobManager;
import Brawler.JobGroup;
import Brawler.Job;
import Util.Coroutine;
import Util.Engine;
import Brawler.CommandQueue;
import Brawler.CommandListType;

namespace Brawler
{
	bool RenderJobManager::RenderJobExecutionEnvironment::AwaitingGPUSubmission() const
	{
		return (RecordingCompletionFlag.load() && !CmdListsSubmitted);
	}

	RenderJobManager::RenderFence::RenderFence() :
		Fence(nullptr),
		NextAvailableValue(1)
	{}

	void RenderJobManager::RenderFence::Initialize()
	{
		Microsoft::WRL::ComPtr<ID3D12Fence> fence{ nullptr };
		CheckHRESULT(Util::Engine::GetD3D12Device().CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

		CheckHRESULT(fence.As(&Fence));
	}

	RenderJobManager::RenderJobManager() :
		mRenderThreadAssigned(false),
		mContinueExecution(true),
		mMasterRenderThreadID(),
		mJobBundleQueue(),
		mRunningJobQueue(),
		mFreeGraphicsContextList(),
		mFreeComputeContextList(),
		mFreeCopyContextList(),
		mGraphicsFence(),
		mComputeFence(),
		mCopyFence()
	{}

	RenderJobManager::~RenderJobManager()
	{
		mContinueExecution.store(false);
	}

	void RenderJobManager::Initialize()
	{
		mGraphicsFence.Initialize();
		mComputeFence.Initialize();
		mCopyFence.Initialize();

		AssignMasterRenderThread();
	}

	RenderEventHandle RenderJobManager::SubmitRenderJobBundle(Brawler::RenderJobBundle&& jobBundle)
	{
		std::unique_ptr<RenderJobBundle> jobBundlePtr{ std::make_unique<RenderJobBundle>(std::move(jobBundle)) };
		std::shared_ptr<std::atomic<bool>> completionFlag{ std::make_shared<std::atomic<bool>>(false) };

		std::unique_ptr<RenderJobBundleSubmission> bundleSubmission{ std::make_unique<RenderJobBundleSubmission>(std::move(jobBundlePtr), completionFlag) };
		while (!mJobBundleQueue.PushBack(std::move(bundleSubmission)));

		return RenderEventHandle{ completionFlag };
	}

	std::thread::id RenderJobManager::GetMasterRenderThreadID() const
	{
		return mMasterRenderThreadID;
	}
	
	void RenderJobManager::AssignMasterRenderThread()
	{
		Brawler::JobGroup assignmentJob{};
		assignmentJob.AddJob([this] ()
		{
			mMasterRenderThreadID = std::this_thread::get_id();
			mRenderThreadAssigned.store(true);

			RunMasterRenderThreadLoop();
		});

		// Execute the job asynchronously to ensure that the calling thread does not
		// pick it up.
		assignmentJob.ExecuteJobsAsync();

		// Wait for some thread to become the master render thread.
		while (!mRenderThreadAssigned.load());
	}

	void RenderJobManager::RunMasterRenderThreadLoop()
	{
		// The primary goal of the master render thread is to handle I_RenderJobs which
		// are submitted by other threads. However, if it has no such requests to handle,
		// then it can steal/execute work from the worker thread queues.

		while (mContinueExecution.load())
		{
			ExtractRenderJobBundlesFromQueue();

			if (!mRunningJobQueue.empty())
				UpdateRunningJobQueue();
			else
				Util::Coroutine::TryExecuteJob();
		}
	}

	bool RenderJobManager::ExtractRenderJobBundlesFromQueue()
	{
		bool bundleExtracted = false;
		std::optional<std::unique_ptr<RenderJobBundleSubmission>> extractedBundle{ mJobBundleQueue.TryPop() };

		while (extractedBundle.has_value())
		{
			bundleExtracted = true;

			std::unique_ptr<RenderJobExecutionEnvironment> environment{ CreateRenderJobExecutionEnvironment(std::move(*extractedBundle))};
			RenderJobExecutionEnvironment* const environmentPtr = environment.get();

			// Push the environment itself into more permanent storage. By using std::unique_ptr, we
			// ensure that the environment instance is allocated on the heap; this makes it easier
			// (i.e., possible) for it to be accessed across multiple threads.
			mRunningJobQueue.push_back(std::move(environment));

			DispatchCommandListRecordingJob(*environmentPtr);

			// Process the next request.
			extractedBundle = mJobBundleQueue.TryPop();
		}

		return bundleExtracted;
	}

	void RenderJobManager::DispatchCommandListRecordingJob(RenderJobManager::RenderJobExecutionEnvironment& environment) const
	{
		// The CPU job system used by the Brawler Engine is really flexible. What we will do
		// is spawn a job which does the following:
		//
		//   1. For each GraphicsJob, ComputeJob, and CopyJob, spawn a separate CPU job for
		//      recording the appropriate command list.
		//
		//   2. Wait for all of these jobs to be executed, meaning that the command lists
		//      have been appropriately recorded and closed.
		//
		//   3. Set environmentPtr->RecordingCompletionFlag to true, letting the master
		//      render thread know that it is okay to submit the command lists. We make
		//      sure that only the master render thread submits command lists in order to
		//      guarantee that they are submitted in the correct order.

		// Before we begin recording the commands for the I_RenderJobs, we should initialize
		// their resource transitions.
		environment.JobBundle->InitializeResourceTransitionManagers();

		Brawler::JobGroup beginRecordingJob{};
		beginRecordingJob.AddJob([this, &environment] ()
		{
			Brawler::JobGroup recordingJobGroup{};

			const std::size_t totalJobCount = (environment.JobBundle->GetGraphicsJobCount() + environment.JobBundle->GetComputeJobCount() + environment.JobBundle->GetCopyJobCount());
			recordingJobGroup.Reserve(totalJobCount);

			std::size_t contextIndex = 0;
			for (auto& graphicsJob : environment.JobBundle->GetGraphicsJobs())
			{
				GraphicsContext& context = *(environment.GraphicsContextArr[contextIndex++]);

				recordingJobGroup.AddJob([&context, &graphicsJob] ()
				{
					graphicsJob.RecordCommands(context);
					context.CloseCommandList();
				});
			}

			contextIndex = 0;
			for (auto& computeJob : environment.JobBundle->GetComputeJobs())
			{
				ComputeContext& context = *(environment.ComputeContextArr[contextIndex++]);

				recordingJobGroup.AddJob([&context, &computeJob] ()
				{
					computeJob.RecordCommands(context);
					context.CloseCommandList();
				});
			}

			contextIndex = 0;
			for (auto& copyJob : environment.JobBundle->GetCopyJobs())
			{
				CopyContext& context = *(environment.CopyContextArr[contextIndex++]);

				recordingJobGroup.AddJob([&context, &copyJob] ()
				{
					copyJob.RecordCommands(context);
					context.CloseCommandList();
				});
			}

			// Have the thread sending these jobs out "wait" for their completion. (This thread
			// can steal jobs and help finish this sooner than later.)
			recordingJobGroup.ExecuteJobs();

			// Notify the master render thread that the command lists have finished being
			// recorded.
			environment.RecordingCompletionFlag.store(true);
		});

		// Have a separate thread dispatch these jobs. That way, the master render thread can
		// continue processing other RenderJobBundle submissions.
		beginRecordingJob.ExecuteJobsAsync();
	}

	void RenderJobManager::UpdateRunningJobQueue()
	{
		// Try to submit the command lists waiting in the running queue. We can do this because
		// ID3D12CommandQueue::ExecuteCommandLists() guarantees that all of the command lists from
		// one call are executed before those of the next call are executed. (See the MSDN at
		// https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandqueue-executecommandlists#remarks
		// for more information.)
		//
		// However, we want to make sure that the fences are signalled in purely incrementing order.
		// So, as soon as we find a RenderJobExecutionEnvironment whose commands are not ready to be
		// submitted, we exit the loop.
		for (auto& environment : mRunningJobQueue)
		{
			if (environment->AwaitingGPUSubmission())
				SubmitCommandListsForRenderJobBundle(*environment);

			else if (!environment->CmdListsSubmitted)
				break;
		}
		
		while (!mRunningJobQueue.empty())
		{
			// Get the RenderJobExecutionEnvironment at the front of the queue. This is the oldest
			// submitted RenderJobBundle, so we want to evaluate the completion of this one.
			RenderJobExecutionEnvironment& oldestEnvironment = *(mRunningJobQueue.front());

			// Check for completion of these render jobs on the GPU.
			const bool graphicsFenceCompleted = (!(oldestEnvironment.GraphicsFenceValue.has_value()) || mGraphicsFence.Fence->GetCompletedValue() >= *(oldestEnvironment.GraphicsFenceValue));
			const bool computeFenceCompleted = (!(oldestEnvironment.ComputeFenceValue.has_value()) || mComputeFence.Fence->GetCompletedValue() >= *(oldestEnvironment.ComputeFenceValue));
			const bool copyFenceCompleted = (!(oldestEnvironment.CopyFenceValue.has_value()) || mCopyFence.Fence->GetCompletedValue() >= *(oldestEnvironment.CopyFenceValue));

			if (graphicsFenceCompleted && computeFenceCompleted && copyFenceCompleted)
			{
				// The GPU has finished executing the commands associated with this RenderJobExecutionEnvironment.
				// So, we should remove it from the queue and process the next one.
				//
				// Before we do that, however, we should set the ExecutionCompletionFlag to true; that way, any
				// threads which were waiting on a RenderEventHandle returned by RenderJobManager::SubmitRenderJobBundle()
				// will know that the commands have been executed.
				oldestEnvironment.ExecutionCompletionFlag->store(true);

				// In addition, we should reset and return all of the GraphicsContexts, ComputeContexts, and
				// CopyContexts used by these I_RenderJobs.
				for (auto&& graphicsContextPtr : oldestEnvironment.GraphicsContextArr)
				{
					graphicsContextPtr->ResetCommandList();
					mFreeGraphicsContextList.push_front(std::move(graphicsContextPtr));
				}

				for (auto&& computeContextPtr : oldestEnvironment.ComputeContextArr)
				{
					computeContextPtr->ResetCommandList();
					mFreeComputeContextList.push_front(std::move(computeContextPtr));
				}

				for (auto&& copyContextPtr : oldestEnvironment.CopyContextArr)
				{
					copyContextPtr->ResetCommandList();
					mFreeCopyContextList.push_front(std::move(copyContextPtr));
				}

				mRunningJobQueue.pop_front();
			}
			else
				// If the oldest I_RenderJobs in the queue have not had their commands executed yet, then we should wait.
				break;
		}
	}

	void RenderJobManager::SubmitCommandListsForRenderJobBundle(RenderJobExecutionEnvironment& environment)
	{
		std::vector<ID3D12CommandList*> cmdListPtrArr{};

		// Submit the graphics jobs.
		if (environment.GraphicsFenceValue.has_value())
		{
			cmdListPtrArr.resize(environment.GraphicsContextArr.size());

			for (std::size_t i = 0; i < environment.GraphicsContextArr.size(); ++i)
				cmdListPtrArr[i] = &(environment.GraphicsContextArr[i]->GetCommandList());

			Brawler::D3D12CommandQueue& directQueue{ Util::Engine::GetCommandQueue(Brawler::CommandListType::DIRECT).GetD3D12CommandQueue() };
			directQueue.ExecuteCommandLists(static_cast<std::uint32_t>(cmdListPtrArr.size()), cmdListPtrArr.data());
			CheckHRESULT(directQueue.Signal(mGraphicsFence.Fence.Get(), *(environment.GraphicsFenceValue)));

			cmdListPtrArr.clear();
		}

		// Submit the compute jobs.
		if (environment.ComputeFenceValue.has_value())
		{
			cmdListPtrArr.resize(environment.ComputeContextArr.size());

			for (std::size_t i = 0; i < environment.ComputeContextArr.size(); ++i)
				cmdListPtrArr[i] = &(environment.ComputeContextArr[i]->GetCommandList());

			Brawler::D3D12CommandQueue& computeQueue{ Util::Engine::GetCommandQueue(Brawler::CommandListType::COMPUTE).GetD3D12CommandQueue() };
			computeQueue.ExecuteCommandLists(static_cast<std::uint32_t>(cmdListPtrArr.size()), cmdListPtrArr.data());
			CheckHRESULT(computeQueue.Signal(mComputeFence.Fence.Get(), *(environment.ComputeFenceValue)));

			cmdListPtrArr.clear();
		}

		// Submit the copy jobs.
		if (environment.CopyFenceValue.has_value())
		{
			cmdListPtrArr.resize(environment.CopyContextArr.size());

			for (std::size_t i = 0; i < environment.CopyContextArr.size(); ++i)
				cmdListPtrArr[i] = &(environment.CopyContextArr[i]->GetCommandList());

			Brawler::D3D12CommandQueue& copyQueue{ Util::Engine::GetCommandQueue(Brawler::CommandListType::COPY).GetD3D12CommandQueue() };
			copyQueue.ExecuteCommandLists(static_cast<std::uint32_t>(cmdListPtrArr.size()), cmdListPtrArr.data());
			CheckHRESULT(copyQueue.Signal(mCopyFence.Fence.Get(), *(environment.CopyFenceValue)));
		}

		environment.CmdListsSubmitted = true;
	}

	std::unique_ptr<RenderJobManager::RenderJobExecutionEnvironment> RenderJobManager::CreateRenderJobExecutionEnvironment(std::unique_ptr<RenderJobBundleSubmission>&& jobBundleSubmission)
	{
		std::unique_ptr<RenderJobExecutionEnvironment> environment{ std::make_unique<RenderJobExecutionEnvironment>() };
		environment->RecordingCompletionFlag.store(false);
		environment->CmdListsSubmitted = false;
		environment->ExecutionCompletionFlag = std::move(jobBundleSubmission->CompletionFlag);

		{
			// Reserve the necessary GraphicsContexts.
			const std::size_t graphicsJobCount = jobBundleSubmission->JobBundle->GetGraphicsJobCount();
			environment->GraphicsContextArr.reserve(graphicsJobCount);

			for (std::size_t i = 0; i < graphicsJobCount; ++i)
			{
				if (!mFreeGraphicsContextList.empty())
				{
					environment->GraphicsContextArr.push_back(std::move(mFreeGraphicsContextList.front()));
					mFreeGraphicsContextList.pop_front();
				}
				else
					environment->GraphicsContextArr.push_back(std::make_unique<GraphicsContext>());
			}

			// If we have at least one graphics job, then prepare to signal the graphics fence.
			if (graphicsJobCount > 0)
				environment->GraphicsFenceValue = (mGraphicsFence.NextAvailableValue)++;
		}

		{
			// Reserve the necessary ComputeContexts.
			const std::size_t computeJobCount = jobBundleSubmission->JobBundle->GetComputeJobCount();
			environment->ComputeContextArr.reserve(computeJobCount);

			for (std::size_t i = 0; i < computeJobCount; ++i)
			{
				if (!mFreeComputeContextList.empty())
				{
					environment->ComputeContextArr.push_back(std::move(mFreeComputeContextList.front()));
					mFreeComputeContextList.pop_front();
				}
				else
					environment->ComputeContextArr.push_back(std::make_unique<ComputeContext>());
			}

			// If we have at least one compute job, then prepare to signal the compute fence.
			if (computeJobCount > 0)
				environment->ComputeFenceValue = (mComputeFence.NextAvailableValue)++;
		}

		{
			// Reserve the necessary CopyContexts.
			const std::size_t copyJobCount = jobBundleSubmission->JobBundle->GetCopyJobCount();
			environment->CopyContextArr.reserve(copyJobCount);

			for (std::size_t i = 0; i < copyJobCount; ++i)
			{
				if (!mFreeCopyContextList.empty())
				{
					environment->CopyContextArr.push_back(std::move(mFreeCopyContextList.front()));
					mFreeCopyContextList.pop_front();
				}
				else
					environment->CopyContextArr.push_back(std::make_unique<CopyContext>());
			}

			// If we have at least one copy job, then prepare to signal the copy fence.
			if (copyJobCount > 0)
				environment->CopyFenceValue = (mCopyFence.NextAvailableValue)++;
		}

		environment->JobBundle = std::move(jobBundleSubmission->JobBundle);
		return environment;
	}
}