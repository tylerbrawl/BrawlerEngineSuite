module;
#include <atomic>
#include <memory>
#include <vector>
#include <optional>
#include <deque>
#include <forward_list>
#include <thread>
#include "DxDef.h"

export module Brawler.RenderJobManager;
import Brawler.RenderJobBundle;
import Brawler.ThreadSafeQueue;
import Brawler.GraphicsContext;
import Brawler.ComputeContext;
import Brawler.CopyContext;
import Brawler.RenderEventHandle;

namespace
{
	static constexpr std::size_t RENDER_JOB_BUNDLE_QUEUE_SIZE = 128;
}

export namespace Brawler
{
	class RenderJobManager
	{
	private:
		struct RenderJobBundleSubmission
		{
			std::unique_ptr<RenderJobBundle> JobBundle;
			std::shared_ptr<std::atomic<bool>> CompletionFlag;
		};

		struct RenderJobExecutionEnvironment
		{
			RenderJobExecutionEnvironment() = default;

			RenderJobExecutionEnvironment(const RenderJobExecutionEnvironment& rhs) = delete;
			RenderJobExecutionEnvironment& operator=(const RenderJobExecutionEnvironment& rhs) = delete;

			RenderJobExecutionEnvironment(RenderJobExecutionEnvironment&& rhs) noexcept = default;
			RenderJobExecutionEnvironment& operator=(RenderJobExecutionEnvironment&& rhs) noexcept = default;

			std::unique_ptr<RenderJobBundle> JobBundle;
			
			std::vector<std::unique_ptr<GraphicsContext>> GraphicsContextArr;
			std::vector<std::unique_ptr<ComputeContext>> ComputeContextArr;
			std::vector<std::unique_ptr<CopyContext>> CopyContextArr;

			// This should be set to true when the command lists have finished being
			// recorded.
			std::atomic<bool> RecordingCompletionFlag;

			// This should be set to true when the command lists have been submitted
			// by the master render thread.
			bool CmdListsSubmitted;
			
			// This should be set to true when the GPU finishes executing the relevant
			// commands.
			std::shared_ptr<std::atomic<bool>> ExecutionCompletionFlag;

			std::optional<std::uint64_t> GraphicsFenceValue;
			std::optional<std::uint64_t> ComputeFenceValue;
			std::optional<std::uint64_t> CopyFenceValue;

			bool AwaitingGPUSubmission() const;
		};

		struct RenderFence
		{
			RenderFence();

			void Initialize();
			
			Microsoft::WRL::ComPtr<Brawler::D3D12Fence> Fence;
			std::uint64_t NextAvailableValue;
		};

	public:
		RenderJobManager();
		~RenderJobManager();

		RenderJobManager(const RenderJobManager& rhs) = delete;
		RenderJobManager& operator=(const RenderJobManager& rhs) = delete;

		RenderJobManager(RenderJobManager&& rhs) noexcept = default;
		RenderJobManager& operator=(RenderJobManager&& rhs) noexcept = default;

		void Initialize();

		/// <summary>
		/// Submits the I_RenderJobs included in jobBundle to the queue of this
		/// RenderJobManager instance. The function does not wait for the jobs
		/// to be executed before returning.
		/// </summary>
		/// <param name="jobBundle">
		/// - The RenderJobBundle whose contents are to be submitted to the queue.
		/// </param>
		/// <returns>
		/// The function returns a RenderEventHandle which can be used to query
		/// the completion of the submitted jobs on the GPU, or to wait for their
		/// completion.
		/// </returns>
		RenderEventHandle SubmitRenderJobBundle(Brawler::RenderJobBundle&& jobBundle);

		std::thread::id GetMasterRenderThreadID() const;

	private:
		/// <summary>
		/// Submits a Brawler::Job for a separate thread to pick up and assume the role of
		/// master render thread. To ensure that the calling thread does not take this
		/// responsibility, this function will block said thread until another thread
		/// becomes the master render thread.
		/// </summary>
		void AssignMasterRenderThread();

		void RunMasterRenderThreadLoop();
		bool ExtractRenderJobBundlesFromQueue();
		void DispatchCommandListRecordingJob(RenderJobExecutionEnvironment& environment) const;
		void UpdateRunningJobQueue();
		void SubmitCommandListsForRenderJobBundle(RenderJobExecutionEnvironment& environment);
		std::unique_ptr<RenderJobExecutionEnvironment> CreateRenderJobExecutionEnvironment(std::unique_ptr<RenderJobBundleSubmission>&& jobBundleSubmission);

	private:
		std::atomic<bool> mRenderThreadAssigned;
		std::atomic<bool> mContinueExecution;
		std::thread::id mMasterRenderThreadID;
		ThreadSafeQueue<std::unique_ptr<RenderJobBundleSubmission>, RENDER_JOB_BUNDLE_QUEUE_SIZE> mJobBundleQueue;
		std::deque<std::unique_ptr<RenderJobExecutionEnvironment>> mRunningJobQueue;
		std::forward_list<std::unique_ptr<GraphicsContext>> mFreeGraphicsContextList;
		std::forward_list<std::unique_ptr<ComputeContext>> mFreeComputeContextList;
		std::forward_list<std::unique_ptr<CopyContext>> mFreeCopyContextList;
		RenderFence mGraphicsFence;
		RenderFence mComputeFence;
		RenderFence mCopyFence;
	};
}