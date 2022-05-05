module;
#include <atomic>
#include <memory>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.GPUCommandManager;
import Brawler.D3D12.GPUCommandListType;
import Brawler.D3D12.GPUCommandContextVault;
import Brawler.D3D12.GPUEventHandle;
import Brawler.JobSystem;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUJobGroup;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandManager
		{
		private:
			struct CommandContextSubmitInfo
			{
				Brawler::D3D12GraphicsCommandList* CmdListPtr;
				Brawler::D3D12Fence* FencePtr;
				std::uint64_t NextFenceValue;
			};

			template <GPUCommandListType CmdListType>
			struct PrepareGPURecordJobsContext;

		public:
			GPUCommandManager() = default;

			GPUCommandManager(const GPUCommandManager& rhs) = delete;
			GPUCommandManager& operator=(const GPUCommandManager& rhs) = delete;

			GPUCommandManager(GPUCommandManager&& rhs) noexcept = default;
			GPUCommandManager& operator=(GPUCommandManager&& rhs) noexcept = default;

			void Initialize();

			GPUEventHandle SubmitGPUJobGroup(GPUJobGroup&& jobGroup);

			template <GPUCommandListType CmdListType>
			Brawler::D3D12CommandQueue& GetCommandQueue();

			template <GPUCommandListType CmdListType>
			const Brawler::D3D12CommandQueue& GetCommandQueue() const;

		private:
			/// <summary>
			/// Waits for the last GPUEventHandle to signal its completion on the GPU
			/// and swaps it with the GPUEventHandle specified by hNextEventHandle.
			/// 
			/// This function is used to ensure that all of the command lists from a
			/// previous GPUJobGroup across all of the queues have finished execution
			/// before the command lists for the next GPUJobGroup are submitted for
			/// execution. It is designed to be thread safe.
			/// </summary>
			/// <param name="hNextEventHandle">
			/// - The GPUEventHandle which will replace the GPUEventHandle representing
			///   the completion of the previous GPUJobGroup's command lists on the GPU.
			/// </param>
			void ExchangeCurrentGPUEventHandle(const GPUEventHandle& hNextEventHandle);

			template <GPUCommandListType CmdListType>
			void PrepareGPURecordJobs(const PrepareGPURecordJobsContext<CmdListType>& context);

			template <GPUCommandListType CmdListType>
			void SubmitCommandListsForExecution(const std::span<CommandContextSubmitInfo> contextSubmitSpan);

		private:
			Microsoft::WRL::ComPtr<Brawler::D3D12CommandQueue> mDirectCmdQueue;
			Microsoft::WRL::ComPtr<Brawler::D3D12CommandQueue> mComputeCmdQueue;
			Microsoft::WRL::ComPtr<Brawler::D3D12CommandQueue> mCopyCmdQueue;
			GPUCommandContextVault mCmdContextVault;
			std::atomic<std::shared_ptr<GPUEventHandle>> mCurrEventHandle;
		};
	}
}

// -------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandListType CmdListType>
		Brawler::D3D12CommandQueue& GPUCommandManager::GetCommandQueue()
		{
			// There's no switch constexpr, sadly.

			if constexpr (CmdListType == GPUCommandListType::DIRECT)
				return *(mDirectCmdQueue.Get());

			if constexpr (CmdListType == GPUCommandListType::COMPUTE)
				return *(mComputeCmdQueue.Get());

			return *(mCopyCmdQueue.Get());
		}

		template <GPUCommandListType CmdListType>
		const Brawler::D3D12CommandQueue& GPUCommandManager::GetCommandQueue() const
		{
			// There's no switch constexpr, sadly.

			if constexpr (CmdListType == GPUCommandListType::DIRECT)
				return *(mDirectCmdQueue.Get());

			if constexpr (CmdListType == GPUCommandListType::COMPUTE)
				return *(mComputeCmdQueue.Get());

			return *(mCopyCmdQueue.Get());
		}
	}
}