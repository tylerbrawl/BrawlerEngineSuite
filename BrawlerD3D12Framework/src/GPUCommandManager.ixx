module;
#include <memory>
#include <array>
#include <span>
#include <mutex>
#include <queue>
#include "DxDef.h"

export module Brawler.D3D12.GPUCommandManager;
import Brawler.D3D12.GPUCommandContextVault;
import Brawler.D3D12.GPUCommandContexts;
import Brawler.D3D12.GPUCommandContextSink;
import Brawler.D3D12.GPUCommandQueue;
import Brawler.CompositeEnum;
import Brawler.D3D12.GPUCommandQueueType;
import Util.Engine;
import Brawler.D3D12.GPUCommandContextSubmissionPoint;
import Brawler.D3D12.GPUCommandContextGroup;

export namespace Brawler
{
	namespace D3D12
	{
		class FrameGraphFenceCollection;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandManager
		{
		private:
			struct GPUCommandContextSinkInfo
			{
				std::queue<FrameGraphFenceCollection*> FenceCollectionQueue;
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

		private:
			void DrainGPUCommandContextSinks(std::size_t beginSinkIndex);
			void EnsureGPUResidencyForCurrentFrame(FrameGraphFenceCollection& fenceCollection) const;

			template <GPUCommandQueueType QueueType>
			const GPUCommandQueue<QueueType>& GetGPUCommandQueue() const;
			
			template <GPUCommandQueueType QueueType>
			void HaltGPUCommandQueueForPreviousSubmission() const;

		private:
			GPUCommandQueue<GPUCommandQueueType::DIRECT> mDirectCmdQueue;
			GPUCommandQueue<GPUCommandQueueType::COMPUTE> mComputeCmdQueue;
			GPUCommandQueue<GPUCommandQueueType::COPY> mCopyCmdQueue;
			Brawler::CompositeEnum<GPUCommandQueueType> mLastSubmissionQueues;
			std::array<GPUCommandContextSink, Util::Engine::MAX_FRAMES_IN_FLIGHT> mCmdContextSinkArr;
			GPUCommandContextSinkInfo mSinkInfo;
			GPUCommandContextVault mCmdContextVault;
		};
	}
}