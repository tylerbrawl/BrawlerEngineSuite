module;
#include <span>
#include <vector>
#include <optional>
#include <cassert>

export module Brawler.D3D12.GPUResourceEventManager;
import Brawler.D3D12.GPUResourceEvent;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.GPUResourceEventCollection;
import Brawler.FastUnorderedMap;

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
		class GPUResourceEventManager
		{
		private:
			struct EventQueueContainer
			{
				std::vector<GPUResourceEvent> EventQueue;
				std::size_t CurrIndex;
				std::size_t EventQueueSize;
			};

		private:
			template <GPUCommandQueueType QueueType>
			using RenderPassEventQueueMap_T = Brawler::FastUnorderedMap<const I_RenderPass<QueueType>*, EventQueueContainer>;

		public:
			GPUResourceEventManager() = default;

			GPUResourceEventManager(const GPUResourceEventManager& rhs) = delete;
			GPUResourceEventManager& operator=(const GPUResourceEventManager& rhs) = delete;

			GPUResourceEventManager(GPUResourceEventManager&& rhs) noexcept = default;
			GPUResourceEventManager& operator=(GPUResourceEventManager&& rhs) noexcept = default;

			template <GPUCommandQueueType QueueType>
			void SetRenderPassCount(const std::size_t numRenderPasses);

			template <GPUCommandQueueType QueueType>
			void AddGPUResourceEvent(const I_RenderPass<QueueType>& renderPass, GPUResourceEvent&& resourceEvent);

			/// <summary>
			/// Extracts the next GPUResourceEvent from the queue corresponding to the I_RenderPass
			/// instance specified by renderPass, if one exists.
			/// 
			/// NOTE: This function is *NOT* entirely thread-safe! It is okay for multiple threads
			/// to call it simultaneously iff they are doing so for different I_RenderPass instances!
			/// Otherwise, you *WILL* have data races and cache coherency issues!
			/// </summary>
			/// <param name="renderPass">
			/// - The I_RenderPass instance for which the next GPUResourceEvent is to be extracted.
			/// </param>
			/// <returns>
			/// If there are any remaining GPUResourceEvent instances for the I_RenderPass specified
			/// by renderPass, then the returned std::optional instance has a valid value, and this
			/// value is the event which was extracted from the corresponding GPUResourceEvent queue.
			/// 
			/// Otherwise, the returned std::optional instance has no value.
			/// </returns>
			template <GPUCommandQueueType QueueType>
			std::optional<GPUResourceEvent> ExtractGPUResourceEvent(const I_RenderPass<QueueType>& renderPass);

			void CountGPUResourceEvents(const GPUResourceEventCollection& eventCollection);
			void AllocateEventQueueMemory();

			void MergeGPUResourceEventCollection(GPUResourceEventCollection&& eventCollection);

		private:
			template <GPUCommandQueueType QueueType>
				requires (QueueType != GPUCommandQueueType::COUNT_OR_ERROR)
			RenderPassEventQueueMap_T<QueueType>& GetRenderPassEventQueueMap();

			template <GPUCommandQueueType QueueType>
				requires (QueueType != GPUCommandQueueType::COUNT_OR_ERROR)
			RenderPassEventQueueMap_T<QueueType>& GetRenderPassEventQueueMap(const I_RenderPass<QueueType>& renderPass);

		private:
			RenderPassEventQueueMap_T<GPUCommandQueueType::DIRECT> mDirectPassQueueMap;
			RenderPassEventQueueMap_T<GPUCommandQueueType::COMPUTE> mComputePassQueueMap;
			RenderPassEventQueueMap_T<GPUCommandQueueType::COPY> mCopyPassQueueMap;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void GPUResourceEventManager::SetRenderPassCount(const std::size_t numRenderPasses)
		{
			// We don't actually need to set anything here. This is just an optimization to try to
			// reserve memory for the maps up front.
			//
			// We specify (1.25 * numRenderPasses) in order to try to reduce the chances of a
			// re-hash occurring, since these can be expensive.
			const float reservationInElements = (1.25f * static_cast<float>(numRenderPasses));
			GetRenderPassEventQueueMap<QueueType>().Reserve(static_cast<std::size_t>(reservationInElements));
		}

		template <GPUCommandQueueType QueueType>
		void GPUResourceEventManager::AddGPUResourceEvent(const I_RenderPass<QueueType>& renderPass, GPUResourceEvent&& resourceEvent)
		{
			RenderPassEventQueueMap_T<QueueType>& eventQueueMap{ GetRenderPassEventQueueMap(renderPass) };
			eventQueueMap[&renderPass].EventQueue.push_back(std::move(resourceEvent));
		}

		template <GPUCommandQueueType QueueType>
		std::optional<GPUResourceEvent> GPUResourceEventManager::ExtractGPUResourceEvent(const I_RenderPass<QueueType>& renderPass)
		{
			RenderPassEventQueueMap_T<QueueType>& eventQueueMap{ GetRenderPassEventQueueMap(renderPass) };

			if (!eventQueueMap.Contains(&renderPass))
				return std::optional<GPUResourceEvent>{};

			EventQueueContainer& eventQueueContainer{ eventQueueMap.At(&renderPass) };

			if (eventQueueContainer.CurrIndex == eventQueueContainer.EventQueue.size())
				return std::optional<GPUResourceEvent>{};

			return std::optional<GPUResourceEvent>{ std::move(eventQueueContainer.EventQueue[eventQueueContainer.CurrIndex++]) };
		}

		void GPUResourceEventManager::CountGPUResourceEvents(const GPUResourceEventCollection& eventCollection)
		{
			const auto countEventsLambda = [this]<GPUCommandQueueType QueueType>(const GPUResourceEventCollection & eventCollection)
			{
				RenderPassEventQueueMap_T<QueueType>& thisEventQueueMap{ GetRenderPassEventQueueMap<QueueType>() };

				for (const auto& eventContainer : eventCollection.GetGPUResourceEvents<QueueType>())
					++(thisEventQueueMap[eventContainer.RenderPassPtr].EventQueueSize);
			};

			countEventsLambda.operator()<GPUCommandQueueType::DIRECT>(eventCollection);
			countEventsLambda.operator()<GPUCommandQueueType::COMPUTE>(eventCollection);
			countEventsLambda.operator()<GPUCommandQueueType::COPY>(eventCollection);
		}

		void GPUResourceEventManager::AllocateEventQueueMemory()
		{
			const auto allocateMemoryLambda = [this]<GPUCommandQueueType QueueType>()
			{
				GetRenderPassEventQueueMap<QueueType>().ForEach([] (EventQueueContainer& eventQueueContainer)
				{
					eventQueueContainer.EventQueue.reserve(eventQueueContainer.EventQueueSize);
				});
			};

			allocateMemoryLambda.operator()<GPUCommandQueueType::DIRECT>();
			allocateMemoryLambda.operator()<GPUCommandQueueType::COMPUTE>();
			allocateMemoryLambda.operator()<GPUCommandQueueType::COPY>();
		}

		void GPUResourceEventManager::MergeGPUResourceEventCollection(GPUResourceEventCollection&& eventCollection)
		{
			const auto mergeMapLambda = [this]<GPUCommandQueueType QueueType>(GPUResourceEventCollection& eventCollection)
			{
				RenderPassEventQueueMap_T<QueueType>& thisEventQueueMap{ GetRenderPassEventQueueMap<QueueType>() };
				const std::span<GPUResourceEventCollection::EventContainer<QueueType>> mergedEventContainerSpan{ eventCollection.GetGPUResourceEvents<QueueType>() };

				for (auto&& eventContainer : mergedEventContainerSpan)
					thisEventQueueMap[eventContainer.RenderPassPtr].EventQueue.push_back(std::move(eventContainer.Event));
			};

			mergeMapLambda.operator()<GPUCommandQueueType::DIRECT>(eventCollection);
			mergeMapLambda.operator()<GPUCommandQueueType::COMPUTE>(eventCollection);
			mergeMapLambda.operator()<GPUCommandQueueType::COPY>(eventCollection);
		}

		template <GPUCommandQueueType QueueType>
			requires (QueueType != GPUCommandQueueType::COUNT_OR_ERROR)
		GPUResourceEventManager::RenderPassEventQueueMap_T<QueueType>& GPUResourceEventManager::GetRenderPassEventQueueMap()
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return mDirectPassQueueMap;

			if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return mComputePassQueueMap;

			if constexpr (QueueType == GPUCommandQueueType::COPY)
				return mCopyPassQueueMap;
		}

		template <GPUCommandQueueType QueueType>
			requires (QueueType != GPUCommandQueueType::COUNT_OR_ERROR)
		GPUResourceEventManager::RenderPassEventQueueMap_T<QueueType>& GPUResourceEventManager::GetRenderPassEventQueueMap(const I_RenderPass<QueueType>& renderPass)
		{
			return GetRenderPassEventQueueMap<QueueType>();
		}
	}
}