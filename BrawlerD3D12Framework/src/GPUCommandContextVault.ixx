module;
#include <memory>
#include <mutex>
#include <ranges>
#include <vector>

export module Brawler.D3D12.GPUCommandContextVault;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.GPUCommandContexts;
import Brawler.D3D12.GPUCommandQueueContextType;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandContextVault
		{
		private:
			template <typename T>
			struct ContextLocker
			{
				std::vector<std::unique_ptr<T>> ContextList;
				std::mutex CritSection;
			};

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
			ContextLocker<GPUCommandQueueContextType<QueueType>>& GetContextLocker();

			template <GPUCommandQueueType QueueType>
			const ContextLocker<GPUCommandQueueContextType<QueueType>>& GetContextLocker() const;

		private:
			ContextLocker<DirectContext> mDirectContextLocker;
			ContextLocker<ComputeContext> mComputeContextLocker;
			ContextLocker<CopyContext> mCopyContextLocker;
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
			using ContextPtr_T = std::unique_ptr<GPUCommandQueueContextType<QueueType>>;
			
			ContextPtr_T contextPtr{ nullptr };
			ContextLocker<GPUCommandQueueContextType<QueueType>>& contextLocker{ GetContextLocker<QueueType>() };

			// Try to acquire a command context from the ContextLocker. We only consider those whose
			// commands were already executed by the GPU.
			{
				std::scoped_lock<std::mutex> lock{ contextLocker.CritSection };

				auto itr = std::ranges::find_if(contextLocker.ContextList, [] (const ContextPtr_T& context) { return context->ReadyForUse(); });
				if (itr != contextLocker.ContextList.end())
				{
					// We found a context which we can use.
					contextPtr = std::move(*itr);
					contextLocker.ContextList.erase(itr);
				}
			}

			// If we do not have a free command context, then we need to create one.
			if (contextPtr == nullptr)
				contextPtr = std::make_unique<GPUCommandQueueContextType<QueueType>>();

			return contextPtr;
		}

		template <GPUCommandQueueType QueueType>
		void GPUCommandContextVault::ReturnCommandContext(std::unique_ptr<GPUCommandQueueContextType<QueueType>>&& cmdContext)
		{
			ContextLocker<GPUCommandQueueContextType<QueueType>>& contextLocker{ GetContextLocker<QueueType>() };

			{
				std::scoped_lock<std::mutex> lock{ contextLocker.CritSection };
				contextLocker.ContextList.push_back(std::move(cmdContext));
			}
		}

		template <GPUCommandQueueType QueueType>
		GPUCommandContextVault::ContextLocker<GPUCommandQueueContextType<QueueType>>& GPUCommandContextVault::GetContextLocker()
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return mDirectContextLocker;

			if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return mComputeContextLocker;

			if constexpr (QueueType == GPUCommandQueueType::COPY)
				return mCopyContextLocker;
		}

		template <GPUCommandQueueType QueueType>
		const GPUCommandContextVault::ContextLocker<GPUCommandQueueContextType<QueueType>>& GPUCommandContextVault::GetContextLocker() const
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return mDirectContextLocker;

			if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return mComputeContextLocker;

			if constexpr (QueueType == GPUCommandQueueType::COPY)
				return mCopyContextLocker;
		}
	}
}