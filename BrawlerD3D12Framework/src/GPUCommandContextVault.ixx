module;
#include <memory>

export module Brawler.D3D12.GPUCommandContextVault;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.GPUCommandContexts;
import Brawler.D3D12.GPUCommandQueueContextType;
import Brawler.ThreadSafeQueue;

namespace Brawler
{
	namespace D3D12
	{
		static constexpr std::size_t CONTEXT_QUEUE_SIZE = 500;
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
			using ContextQueue = Brawler::ThreadSafeQueue<std::unique_ptr<T>, CONTEXT_QUEUE_SIZE>;

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

// -------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		std::unique_ptr<GPUCommandQueueContextType<QueueType>> GPUCommandContextVault::AcquireCommandContext()
		{
			// Try to acquire a command context from the ContextQueue.
			std::optional<std::unique_ptr<GPUCommandQueueContextType<QueueType>>> contextPtr{ GetContextQueue<QueueType>().TryPop() };

			// If we do not have a free command context, then we need to create one.
			return (contextPtr.has_value() ? std::move(*contextPtr) : std::make_unique<GPUCommandQueueContextType<QueueType>>());
		}

		template <GPUCommandQueueType QueueType>
		void GPUCommandContextVault::ReturnCommandContext(std::unique_ptr<GPUCommandQueueContextType<QueueType>>&& cmdContext)
		{
			// We can just ignore the return result of ThreadSafeQueue::TryPop(). A GPUCommandContext
			// instance only contains a closed command list at this point.
			// 
			// TODO: Is it okay to just destroy an ID3D12GraphicsCommandList object immediately after
			// closing it? It's the ID3D12CommandAllocator which stores the commands, so it should be
			// fine.
			std::ignore = GetContextQueue<QueueType>().PushBack(std::move(cmdContext));
		}

		template <GPUCommandQueueType QueueType>
		auto& GPUCommandContextVault::GetContextQueue()
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return mDirectContextQueue;

			if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return mComputeContextQueue;

			if constexpr (QueueType == GPUCommandQueueType::COPY)
				return mCopyContextQueue;
		}

		template <GPUCommandQueueType QueueType>
		const auto& GPUCommandContextVault::GetContextQueue() const
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return mDirectContextQueue;

			if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return mComputeContextQueue;

			if constexpr (QueueType == GPUCommandQueueType::COPY)
				return mCopyContextQueue;
		}
	}
}