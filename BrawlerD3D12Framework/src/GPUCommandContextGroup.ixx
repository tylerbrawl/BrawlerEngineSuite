module;
#include <vector>
#include <memory>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.GPUCommandContextGroup;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.GPUCommandContexts;
import Brawler.CompositeEnum;
import Brawler.D3D12.GPUCommandQueueContextType;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandContextGroup
		{
		private:
			template <GPUCommandQueueType QueueType>
			using UniqueContextPtr = std::unique_ptr<GPUCommandQueueContextType<QueueType>>;

		public:
			GPUCommandContextGroup() = default;

			GPUCommandContextGroup(const GPUCommandContextGroup& rhs) = delete;
			GPUCommandContextGroup& operator=(const GPUCommandContextGroup& rhs) = delete;

			GPUCommandContextGroup(GPUCommandContextGroup&& rhs) noexcept = default;
			GPUCommandContextGroup& operator=(GPUCommandContextGroup&& rhs) noexcept = default;

			Brawler::CompositeEnum<GPUCommandQueueType> GetUsedQueues() const;

			template <GPUCommandQueueType QueueType>
			void AddGPUCommandContexts(std::span<UniqueContextPtr<QueueType>> contextSpan);

			template <GPUCommandQueueType QueueType>
			std::span<UniqueContextPtr<QueueType>> GetGPUCommandContexts();

		private:
			template <GPUCommandQueueType QueueType>
			std::vector<UniqueContextPtr<QueueType>>& GetGPUCommandContextArray();

		private:
			std::vector<UniqueContextPtr<GPUCommandQueueType::DIRECT>> mDirectContextArr;
			std::vector<UniqueContextPtr<GPUCommandQueueType::COMPUTE>> mComputeContextArr;
			std::vector<UniqueContextPtr<GPUCommandQueueType::COPY>> mCopyContextArr;
			Brawler::CompositeEnum<GPUCommandQueueType> mUsedQueuesTracker;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void GPUCommandContextGroup::AddGPUCommandContexts(std::span<UniqueContextPtr<QueueType>> contextSpan)
		{
			std::vector<UniqueContextPtr<QueueType>>& cmdContextArr{ GetGPUCommandContextArray<QueueType>() };
			cmdContextArr.reserve(cmdContextArr.size() + contextSpan.size());

			for (auto&& cmdContext : contextSpan)
			{
				// Only mark a queue as being used if it actually is going to be sending any
				// commands to the GPU.

				if (cmdContext->HasCommands())
					mUsedQueuesTracker |= QueueType;

				cmdContextArr.push_back(std::move(cmdContext));
			}
		}

		template <GPUCommandQueueType QueueType>
		std::span<GPUCommandContextGroup::UniqueContextPtr<QueueType>> GPUCommandContextGroup::GetGPUCommandContexts()
		{
			return std::span<UniqueContextPtr<QueueType>>{ GetGPUCommandContextArray<QueueType>() };
		}

		template <GPUCommandQueueType QueueType>
		std::vector<GPUCommandContextGroup::UniqueContextPtr<QueueType>>& GPUCommandContextGroup::GetGPUCommandContextArray()
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return mDirectContextArr;

			if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return mComputeContextArr;

			if constexpr (QueueType == GPUCommandQueueType::COPY)
				return mCopyContextArr;
		}
	}
}