module;
#include <variant>
#include <optional>
#include <array>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceStateBarrierMerger;
import Brawler.D3D12.GPUCommandQueueType;
import Util.D3D12;
import Brawler.D3D12.GPUResourceEvent;
import Brawler.D3D12.GPUResourceEventManager;

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
		class GPUResourceStateBarrierMerger
		{
		private:
			struct RenderPassInfo
			{
				std::variant<const I_RenderPass<GPUCommandQueueType::DIRECT>*, const I_RenderPass<GPUCommandQueueType::COMPUTE>*, const I_RenderPass<GPUCommandQueueType::COPY>*> RenderPass;
				GPUCommandQueueType QueueType;

				void AddGPUResourceEventToEventManager(GPUResourceEventManager& eventManager, GPUResourceEvent&& resourceEvent) const;
				bool HasSameRenderPass(const RenderPassInfo& otherPassInfo) const;
			};

		public:
			GPUResourceStateBarrierMerger(I_GPUResource& resource, GPUResourceEventManager& eventManager);

			GPUResourceStateBarrierMerger(const GPUResourceStateBarrierMerger& rhs) = delete;
			GPUResourceStateBarrierMerger& operator=(const GPUResourceStateBarrierMerger& rhs) = delete;

			GPUResourceStateBarrierMerger(GPUResourceStateBarrierMerger&& rhs) noexcept = default;
			GPUResourceStateBarrierMerger& operator=(GPUResourceStateBarrierMerger&& rhs) noexcept = default;

			template <GPUCommandQueueType QueueType>
			void AddPotentialSplitBarrierBeginRenderPass(const I_RenderPass<QueueType>& renderPass);

			template <GPUCommandQueueType QueueType>
			void AddExplicitStateTransition(const I_RenderPass<QueueType>& renderPass, const D3D12_RESOURCE_STATES requiredState);

			void CommitExistingExplicitStateTransition();

			void ErasePotentialSplitBarrierBeginRenderPasses();

		private:
			bool ResourceNeedsSpecialInitialization() const;
			void MarkSpecialInitializationAsCompleted();

			D3D12_RESOURCE_STATES GetCurrentResourceState() const;

		private:
			I_GPUResource* mResourcePtr;
			GPUResourceEventManager* mEventManagerPtr;
			D3D12_RESOURCE_STATES mNextState;
			std::array<std::optional<RenderPassInfo>, 3> mPotentialBeginBarrierPassArr;
			std::optional<RenderPassInfo> mEndBarrierPass;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void GPUResourceStateBarrierMerger::AddPotentialSplitBarrierBeginRenderPass(const I_RenderPass<QueueType>& renderPass)
		{
			if (ResourceNeedsSpecialInitialization()) [[unlikely]]
				return;
			
			// Add the first of each type of I_RenderPass which is sent to this function.
			
			for (auto& renderPassInfo : mPotentialBeginBarrierPassArr)
			{
				if (!renderPassInfo.has_value())
					renderPassInfo = RenderPassInfo{
						.RenderPass{std::addressof(renderPass)},
						.QueueType = QueueType
					};

				else if (renderPassInfo->QueueType == QueueType)
					return;
			}
		}

		template <GPUCommandQueueType QueueType>
		void GPUResourceStateBarrierMerger::AddExplicitStateTransition(const I_RenderPass<QueueType>& renderPass, const D3D12_RESOURCE_STATES requiredState)
		{
			if (ResourceNeedsSpecialInitialization()) [[unlikely]]
			{
				mEventManagerPtr->AddGPUResourceEvent(renderPass, GPUResourceEvent{
					.GPUResource{mResourcePtr},
					.Event{ResourceInitializationEvent{}},
					.EventID = GPUResourceEventID::RESOURCE_INITIALIZATION
					});

				MarkSpecialInitializationAsCompleted();
			}

			if (GetCurrentResourceState() == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS && GetCurrentResourceState() == requiredState)
				mEventManagerPtr->AddGPUResourceEvent(renderPass, GPUResourceEvent{
					.GPUResource{mResourcePtr},
					.Event{UAVBarrierEvent{}},
					.EventID = GPUResourceEventID::UAV_BARRIER
				});
			
			if ((requiredState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON && mNextState != D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON) ||
				!Util::D3D12::IsResourceStateValid(mNextState | requiredState))
				CommitExistingExplicitStateTransition();

			mNextState |= requiredState;

			if (!mEndBarrierPass.has_value())
				mEndBarrierPass = RenderPassInfo{
					.RenderPass{std::addressof(renderPass)},
					.QueueType = QueueType
				};
		}
	}
}