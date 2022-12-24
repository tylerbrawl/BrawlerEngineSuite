module;
#include <memory>
#include <array>
#include <span>
#include <mutex>
#include <queue>
#include "DxDef.h"

export module Brawler.D3D12.GPUCommandManager;
import Brawler.D3D12.GPUCommandQueue;
import Brawler.D3D12.GPUCommandContextGroup;
import Brawler.CompositeEnum;
import Brawler.D3D12.GPUCommandQueueType;
import Util.Engine;
import Brawler.D3D12.FrameGraphFenceCollection;
import Brawler.ThreadSafeQueue;
import Brawler.D3D12.GPUCommandQueueContextType;
import Brawler.D3D12.GPUCommandContexts;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandContextSink;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandContextSubmissionPoint
		{
		private:
			friend class GPUCommandContextSink;

		public:
			GPUCommandContextSubmissionPoint() = default;

			GPUCommandContextSubmissionPoint(const GPUCommandContextSubmissionPoint& rhs) = delete;
			GPUCommandContextSubmissionPoint& operator=(const GPUCommandContextSubmissionPoint& rhs) = delete;

			GPUCommandContextSubmissionPoint(GPUCommandContextSubmissionPoint&& rhs) noexcept = default;
			GPUCommandContextSubmissionPoint& operator=(GPUCommandContextSubmissionPoint&& rhs) noexcept = default;

			void SubmitGPUCommandContextGroups(std::vector<GPUCommandContextGroup>&& contextGroupSpan);
			std::vector<GPUCommandContextGroup> ExtractGPUCommandContextGroups();

		private:
			void SetSinkNotifier(std::atomic<std::uint64_t>& sinkNotifier);

		private:
			std::vector<GPUCommandContextGroup> mCmdContextGroupArr;
			std::atomic<std::uint64_t>* mSinkNotifierPtr;
			mutable std::mutex mCritSection;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandContextSink
		{
		public:
			GPUCommandContextSink() = default;

			GPUCommandContextSink(const GPUCommandContextSink& rhs) = delete;
			GPUCommandContextSink& operator=(const GPUCommandContextSink& rhs) = delete;

			GPUCommandContextSink(GPUCommandContextSink&& rhs) noexcept = default;
			GPUCommandContextSink& operator=(GPUCommandContextSink&& rhs) noexcept = default;

			std::span<GPUCommandContextSubmissionPoint> InitializeSinkForCurrentFrame(const std::size_t numExecutionModules);

			/// <summary>
			/// When this function is called, the thread which calls it will loop until
			/// all of the GPUCommandContexts for this frame have been sent to the
			/// GPUCommandManager.
			/// 
			/// NOTE: Util::Coroutine::TryExecuteJob() is *NOT* called within this function.
			/// The reason is that if the thread within this function happens to pick up a
			/// long-running job, then the GPU could potentially be left starved of work to
			/// execute.
			/// 
			/// Instead, the thread will use a std::atomic wait to let the OS notify it of
			/// when useful work can be done. (This is more performant and less CPU hungry
			/// than a busy wait.)
			/// </summary>
			void RunGPUSubmissionLoop();

		private:
			std::vector<GPUCommandContextSubmissionPoint> mSubmitPointArr;
			std::atomic<std::uint64_t> mSinkNotifier;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandContextVault
		{
		private:
			template <typename T>
			using ContextQueue = Brawler::ThreadSafeQueue<std::unique_ptr<T>, 500>;

		public:
			GPUCommandContextVault() = default;

			GPUCommandContextVault(const GPUCommandContextVault& rhs) = delete;
			GPUCommandContextVault& operator=(const GPUCommandContextVault& rhs) = delete;

			GPUCommandContextVault(GPUCommandContextVault&& rhs) noexcept = default;
			GPUCommandContextVault& operator=(GPUCommandContextVault&& rhs) noexcept = default;

			/// <summary>
			/// If possible, this function will return a previous command context whose commands
			/// have already been executed by the GPU. Otherwise, this function creates a new
			/// command context.
			/// 
			/// Regardless of what happens, the returned command context can be immediately used
			/// for recording GPU commands.
			/// </summary>
			/// <returns>
			/// The function returns a command context which can be immediately used for recording
			/// GPU commands.
			/// </returns>
			template <GPUCommandQueueType QueueType>
			std::unique_ptr<GPUCommandQueueContextType<QueueType>> AcquireCommandContext();

			/// <summary>
			/// Stores the provided command context back into this GPUCommandContextVault. 
			/// 
			/// It is safe to do this even before the GPU has finished executing its commands; the
			/// GPUCommandContextVault will never return a command context whose commands have
			/// not been executed when GPUCommandContextVault::AcquireCommandContext() is called.
			/// 
			/// Returning command contexts is important to ensure that we only create additional
			/// command allocators as needed.
			/// </summary>
			/// <param name="cmdContext">
			/// - The command context which is to be returned to this GPUCommandContextVault.
			/// </param>
			template <GPUCommandQueueType QueueType>
			void ReturnCommandContext(std::unique_ptr<GPUCommandQueueContextType<QueueType>>&& cmdContext);

		private:
			template <GPUCommandQueueType QueueType>
			auto& GetContextQueue();

			template <GPUCommandQueueType QueueType>
			const auto& GetContextQueue() const;

		private:
			ContextQueue<DirectContext> mDirectContextQueue;
			ContextQueue<ComputeContext> mComputeContextQueue;
			ContextQueue<CopyContext> mCopyContextQueue;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandManager
		{
		private:
			struct FrameGraphSubmissionInfo
			{
				FrameGraphFenceCollection* FenceCollectionPtr;
				std::uint64_t FrameNumber;
			};

			struct GPUCommandContextSinkInfo
			{
				std::queue<FrameGraphSubmissionInfo> SubmissionInfoQueue;
				bool IsThreadHandlingSinks;
				
				mutable std::mutex CritSection;
			};

		public:
			GPUCommandManager() = default;

			GPUCommandManager(const GPUCommandManager& rhs) = delete;
			GPUCommandManager& operator=(const GPUCommandManager& rhs) = delete;

			GPUCommandManager(GPUCommandManager&& rhs) noexcept = default;
			GPUCommandManager& operator=(GPUCommandManager&& rhs) noexcept = default;

			void Initialize();

			std::span<GPUCommandContextSubmissionPoint> BeginGPUCommandContextSubmission(FrameGraphFenceCollection& fenceCollection, const std::size_t numExecutionModules);
			void SubmitGPUCommandContextGroup(GPUCommandContextGroup&& cmdContextGroup);

			GPUCommandContextVault& GetGPUCommandContextVault();

			template <GPUCommandQueueType QueueType>
			const GPUCommandQueue<QueueType>& GetGPUCommandQueue() const;

		private:
			void DrainGPUCommandContextSinks(std::size_t beginSinkIndex);
			void EnsureGPUResidencyForCurrentFrame(FrameGraphFenceCollection& fenceCollection) const;
			
			template <GPUCommandQueueType QueueType>
			void HaltGPUCommandQueueForPreviousSubmission() const;

		private:
			GPUCommandQueue<GPUCommandQueueType::DIRECT> mDirectCmdQueue;
			GPUCommandQueue<GPUCommandQueueType::COMPUTE> mComputeCmdQueue;
			GPUCommandQueue<GPUCommandQueueType::COPY> mCopyCmdQueue;
			Brawler::CompositeEnum<GPUCommandQueueType> mLastSubmissionQueues;
			std::array<GPUCommandContextSink, Util::Engine::MAX_FRAMES_IN_FLIGHT> mCmdContextSinkArr;
			GPUCommandContextSinkInfo mSinkInfo;
			std::unique_ptr<GPUCommandContextVault> mCmdContextVaultPtr;
		};
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

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
	}
}