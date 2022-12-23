module;
#include <span>
#include <memory>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.GPUCommandQueue;
import Brawler.D3D12.GPUFence;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.GPUCommandContextGroup;
import Brawler.D3D12.GPUCommandContexts;
import Util.Engine;
import Util.General;
import Brawler.D3D12.GPUCommandQueueContextType;

export namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		class GPUCommandQueue
		{
		public:
			GPUCommandQueue() = default;

			GPUCommandQueue(const GPUCommandQueue& rhs) = delete;
			GPUCommandQueue& operator=(const GPUCommandQueue& rhs) = delete;

			GPUCommandQueue(GPUCommandQueue&& rhs) noexcept = default;
			GPUCommandQueue& operator=(GPUCommandQueue&& rhs) noexcept = default;

			void Initialize();

			void SubmitGPUCommandContextGroup(GPUCommandContextGroup& cmdContextGroup);

			/// <summary>
			/// Instructs this GPUCommandQueue instance to wait for the fence owned by
			/// cmdQueue to be signalled before it can proceed executing commands on the
			/// GPU timeline.
			/// 
			/// Let cmdQueueA and cmdQueueB be two GPUCommandQueue instances. Then, calling
			/// cmdQueueA.WaitForGPUCommandQueue(cmdQueueB) will prevent cmdQueueA from
			/// executing any further commands on the GPU timeline until all of cmdQueueB's
			/// commands have been executed on the GPU timeline.
			/// </summary>
			/// <param name="cmdQueue">
			/// - The GPUCommandQueue instance with which this GPUCommandQueue instance is
			///   to be synchronized on the GPU timeline.
			/// </param>
			template <GPUCommandQueueType RHSQueueType>
			void WaitForGPUCommandQueue(const GPUCommandQueue<RHSQueueType>& cmdQueue) const;

			GPUFence& GetGPUFence();
			const GPUFence& GetGPUFence() const;

			Brawler::D3D12CommandQueue& GetD3D12CommandQueue() const;

		private:
			Microsoft::WRL::ComPtr<Brawler::D3D12CommandQueue> mCmdQueue;
			GPUFence mFence;
		};
	}
}

// -------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		struct QueueInfo
		{
			static_assert(sizeof(QueueType) != sizeof(QueueType));
		};

		template <D3D12_COMMAND_LIST_TYPE CmdListType>
		struct QueueInfoInstantiation
		{
			static constexpr D3D12_COMMAND_QUEUE_DESC QUEUE_DESC{
				.Type = CmdListType,
				.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
				.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE,
				.NodeMask = 0
			};
		};

		template <>
		struct QueueInfo<GPUCommandQueueType::DIRECT> : public QueueInfoInstantiation<D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT>
		{};

		template <>
		struct QueueInfo<GPUCommandQueueType::COMPUTE> : public QueueInfoInstantiation<D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE>
		{};

		template <>
		struct QueueInfo<GPUCommandQueueType::COPY> : public QueueInfoInstantiation<D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY>
		{};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void GPUCommandQueue<QueueType>::Initialize()
		{
			// Create the command queue.
			Util::General::CheckHRESULT(Util::Engine::GetD3D12Device().CreateCommandQueue(
				&(QueueInfo<QueueType>::QUEUE_DESC),
				IID_PPV_ARGS(&mCmdQueue)
			));

			// Create the fence.
			mFence.Initialize();
		}

		template <GPUCommandQueueType QueueType>
		void GPUCommandQueue<QueueType>::SubmitGPUCommandContextGroup(GPUCommandContextGroup& cmdContextGroup)
		{
			assert(mCmdQueue != nullptr && "ERROR: A GPUCommandQueue was never initialized by calling GPUCommandQueue::Initialize()!");
			
			// Gather all of the ID3D12GraphicsCommandList instances from the GPUCommandContexts.
			const std::span<std::unique_ptr<GPUCommandQueueContextType<QueueType>>> cmdContextSpan{ cmdContextGroup.GetGPUCommandContexts<QueueType>() };

			// Exit early if there are no command lists to submit for this GPUCommandQueue.
			if (cmdContextSpan.empty())
				return;

			std::vector<ID3D12CommandList*> cmdListArr{};
			cmdListArr.reserve(cmdContextSpan.size());

			for (auto& cmdContext : cmdContextSpan)
				cmdListArr.push_back(&(cmdContext->GetCommandList()));

			// Execute the command lists, and then signal the fence on the GPU timeline.
			mCmdQueue->ExecuteCommandLists(static_cast<std::uint32_t>(cmdListArr.size()), cmdListArr.data());
			mFence.SignalOnGPUTimeline(*(mCmdQueue.Get()));
		}

		template <GPUCommandQueueType QueueType>
		template <GPUCommandQueueType RHSQueueType>
		void GPUCommandQueue<QueueType>::WaitForGPUCommandQueue(const GPUCommandQueue<RHSQueueType>& cmdQueue) const
		{
			assert(mCmdQueue != nullptr && "ERROR: A GPUCommandQueue was never initialized by calling GPUCommandQueue::Initialize()!");
			cmdQueue.GetGPUFence().WaitOnGPUTimeline(GetD3D12CommandQueue());
		}

		template <GPUCommandQueueType QueueType>
		GPUFence& GPUCommandQueue<QueueType>::GetGPUFence()
		{
			return mFence;
		}

		template <GPUCommandQueueType QueueType>
		const GPUFence& GPUCommandQueue<QueueType>::GetGPUFence() const
		{
			return mFence;
		}

		template <GPUCommandQueueType QueueType>
		Brawler::D3D12CommandQueue& GPUCommandQueue<QueueType>::GetD3D12CommandQueue() const
		{
			assert(mCmdQueue != nullptr && "ERROR: A GPUCommandQueue was never initialized by calling GPUCommandQueue::Initialize()!");
			return *(mCmdQueue.Get());
		}
	}
}