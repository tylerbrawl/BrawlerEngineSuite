module;
#include <vector>
#include <span>

export module Brawler.D3D12.GPUResourceEventCollection;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.GPUResourceEvent;

export namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		class I_RenderPass;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceEventCollection
		{
		public:
			template <GPUCommandQueueType QueueType>
			struct EventContainer
			{
				GPUResourceEvent Event;
				const I_RenderPass<QueueType>* RenderPassPtr;
			};

		public:
			GPUResourceEventCollection() = default;

			GPUResourceEventCollection(const GPUResourceEventCollection& rhs) = delete;
			GPUResourceEventCollection& operator=(const GPUResourceEventCollection& rhs) = delete;

			GPUResourceEventCollection(GPUResourceEventCollection&& rhs) noexcept = default;
			GPUResourceEventCollection& operator=(GPUResourceEventCollection&& rhs) noexcept = default;

			template <GPUCommandQueueType QueueType>
			void AddGPUResourceEvent(const I_RenderPass<QueueType>& renderPass, GPUResourceEvent&& event);

			template <GPUCommandQueueType QueueType>
			std::span<EventContainer<QueueType>> GetGPUResourceEvents();

			template <GPUCommandQueueType QueueType>
			std::span<const EventContainer<QueueType>> GetGPUResourceEvents() const;

			void MergeGPUResourceEventCollection(GPUResourceEventCollection&& mergedCollection);

		private:
			std::vector<EventContainer<GPUCommandQueueType::DIRECT>> mDirectEventArr;
			std::vector<EventContainer<GPUCommandQueueType::COMPUTE>> mComputeEventArr;
			std::vector<EventContainer<GPUCommandQueueType::COPY>> mCopyEventArr;
		};
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void GPUResourceEventCollection::AddGPUResourceEvent(const I_RenderPass<QueueType>& renderPass, GPUResourceEvent&& event)
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				mDirectEventArr.emplace_back(std::move(event), std::addressof(renderPass));

			else if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				mComputeEventArr.emplace_back(std::move(event), std::addressof(renderPass));

			else
				mCopyEventArr.emplace_back(std::move(event), std::addressof(renderPass));
		}

		template <GPUCommandQueueType QueueType>
		std::span<GPUResourceEventCollection::EventContainer<QueueType>> GPUResourceEventCollection::GetGPUResourceEvents()
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return std::span<EventContainer<QueueType>>{ mDirectEventArr };

			else if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return std::span<EventContainer<QueueType>>{ mComputeEventArr };

			else
				return std::span<EventContainer<QueueType>>{ mCopyEventArr };
		}

		template <GPUCommandQueueType QueueType>
		std::span<const GPUResourceEventCollection::EventContainer<QueueType>> GPUResourceEventCollection::GetGPUResourceEvents() const
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return std::span<const EventContainer<QueueType>>{ mDirectEventArr };

			else if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return std::span<const EventContainer<QueueType>>{ mComputeEventArr };

			else
				return std::span<const EventContainer<QueueType>>{ mCopyEventArr };
		}
	}
}