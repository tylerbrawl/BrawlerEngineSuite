module;
#include <unordered_map>
#include <span>
#include <vector>
#include <optional>
#include <cassert>

export module Brawler.D3D12.GPUResourceEventManager;
import Brawler.D3D12.GPUResourceEvent;
import Brawler.D3D12.GPUCommandQueueType;

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
			};

		private:
			template <GPUCommandQueueType QueueType>
			using RenderPassEventQueueMap_T = std::unordered_map<const I_RenderPass<QueueType>*, EventQueueContainer>;

		public:
			GPUResourceEventManager() = default;

			GPUResourceEventManager(const GPUResourceEventManager& rhs) = delete;
			GPUResourceEventManager& operator=(const GPUResourceEventManager& rhs) = delete;

			GPUResourceEventManager(GPUResourceEventManager&& rhs) noexcept = default;
			GPUResourceEventManager& operator=(GPUResourceEventManager&& rhs) noexcept = default;

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

			void MergeGPUResourceEventManager(GPUResourceEventManager&& eventManagerToMerge);

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
		void GPUResourceEventManager::AddGPUResourceEvent(const I_RenderPass<QueueType>& renderPass, GPUResourceEvent&& resourceEvent)
		{
			RenderPassEventQueueMap_T<QueueType>& eventQueueMap{ GetRenderPassEventQueueMap(renderPass) };
			eventQueueMap[&renderPass].EventQueue.push_back(std::move(resourceEvent));
		}

		template <GPUCommandQueueType QueueType>
		std::optional<GPUResourceEvent> GPUResourceEventManager::ExtractGPUResourceEvent(const I_RenderPass<QueueType>& renderPass)
		{
			RenderPassEventQueueMap_T<QueueType>& eventQueueMap{ GetRenderPassEventQueueMap(renderPass) };

			if (!eventQueueMap.contains(&renderPass))
				return std::optional<GPUResourceEvent>{};

			EventQueueContainer& eventQueueContainer{ eventQueueMap.at(&renderPass) };

			if (eventQueueContainer.CurrIndex == eventQueueContainer.EventQueue.size())
				return std::optional<GPUResourceEvent>{};

			return std::optional<GPUResourceEvent>{ std::move(eventQueueContainer.EventQueue[eventQueueContainer.CurrIndex++]) };
		}

		void GPUResourceEventManager::MergeGPUResourceEventManager(GPUResourceEventManager&& eventManagerToMerge)
		{
			const auto mergeMapLambda = [this]<GPUCommandQueueType QueueType>(GPUResourceEventManager& mergedEventManager)
			{
				RenderPassEventQueueMap_T<QueueType>& thisEventQueueMap{ GetRenderPassEventQueueMap<QueueType>() };
				RenderPassEventQueueMap_T<QueueType>& mergedEventQueueMap{ mergedEventManager.GetRenderPassEventQueueMap<QueueType>() };

				for (auto&& [renderPass, mergedEventQueueContainer] : mergedEventQueueMap)
				{
					std::vector<GPUResourceEvent>& thisEventQueue{ thisEventQueueMap[renderPass].EventQueue };
					thisEventQueue.reserve(mergedEventQueueContainer.EventQueue.size());

					for (auto&& resourceEvent : mergedEventQueueContainer.EventQueue)
						thisEventQueue.push_back(std::move(resourceEvent));
				}
			};

			mergeMapLambda.operator()<GPUCommandQueueType::DIRECT>(eventManagerToMerge);
			mergeMapLambda.operator()<GPUCommandQueueType::COMPUTE>(eventManagerToMerge);
			mergeMapLambda.operator()<GPUCommandQueueType::COPY>(eventManagerToMerge);
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