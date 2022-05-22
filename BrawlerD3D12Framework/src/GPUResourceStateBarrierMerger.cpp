module;
#include <variant>
#include <optional>
#include <array>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceStateBarrierMerger;
import Brawler.D3D12.I_GPUResource;

namespace Brawler
{
	namespace D3D12
	{
		void GPUResourceStateBarrierMerger::RenderPassInfo::AddGPUResourceEventToEventManager(GPUResourceEventManager& eventManager, GPUResourceEvent&& resourceEvent) const
		{
			switch (QueueType)
			{
			case GPUCommandQueueType::DIRECT:
			{
				eventManager.AddGPUResourceEvent(*(std::get<0>(RenderPass)), std::move(resourceEvent));
				break;
			}

			case GPUCommandQueueType::COMPUTE:
			{
				eventManager.AddGPUResourceEvent(*(std::get<1>(RenderPass)), std::move(resourceEvent));
				break;
			}

			case GPUCommandQueueType::COPY:
			{
				eventManager.AddGPUResourceEvent(*(std::get<2>(RenderPass)), std::move(resourceEvent));
				break;
			}
			}
		}

		GPUResourceStateBarrierMerger::GPUResourceStateBarrierMerger(I_GPUResource& resource, GPUResourceEventManager& eventManager) :
			mResourcePtr(&resource),
			mEventManagerPtr(&eventManager),
			mNextState(),
			mPotentialBeginBarrierPassArr(),
			mEndBarrierPass()
		{}

		void GPUResourceStateBarrierMerger::CommitExistingExplicitStateTransition()
		{
			if (!mEndBarrierPass.has_value())
				return;
			
			// Try to find the earliest render pass whose queue can handle a begin split
			// barrier transition.
			Util::D3D12::ResourceTransitionCheckInfo transitionCheckInfo{
				.QueueType{},
				.BeforeState{mResourcePtr->GetCurrentResourceState()},
				.AfterState{mNextState}
			};
			bool isSplitBarrier = false;

			for (const auto& renderPassInfo : mPotentialBeginBarrierPassArr)
			{
				if (renderPassInfo.has_value())
				{
					transitionCheckInfo.QueueType = renderPassInfo->QueueType;
					if (Util::D3D12::CanQueuePerformResourceTransition(transitionCheckInfo))
					{
						renderPassInfo->AddGPUResourceEventToEventManager(*mEventManagerPtr, GPUResourceEvent{
							.GPUResource = mResourcePtr,
							.Event{ResourceTransitionEvent{
								.BeforeState = transitionCheckInfo.BeforeState,
								.AfterState = transitionCheckInfo.AfterState,
								.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY
							}},
							.EventID = GPUResourceEventID::RESOURCE_TRANSITION
						});
						isSplitBarrier = true;

						break;
					}
				}
			}

			mEndBarrierPass->AddGPUResourceEventToEventManager(*mEventManagerPtr, GPUResourceEvent{
				.GPUResource = mResourcePtr,
				.Event{ResourceTransitionEvent{
					.BeforeState = transitionCheckInfo.BeforeState,
					.AfterState = transitionCheckInfo.AfterState,
					.Flags = (isSplitBarrier ? D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_END_ONLY : D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE)
				}},
				.EventID = GPUResourceEventID::RESOURCE_TRANSITION
			});

			ErasePotentialSplitBarrierBeginRenderPasses();
			mResourcePtr->SetCurrentResourceState(mNextState);
			mNextState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
			mEndBarrierPass.reset();
		}

		void GPUResourceStateBarrierMerger::ErasePotentialSplitBarrierBeginRenderPasses()
		{
			for (auto& renderPassInfo : mPotentialBeginBarrierPassArr)
				renderPassInfo.reset();
		}

		bool GPUResourceStateBarrierMerger::ResourceNeedsSpecialInitialization() const
		{
			return mResourcePtr->RequiresSpecialInitialization();
		}

		void GPUResourceStateBarrierMerger::MarkSpecialInitializationAsCompleted()
		{
			mResourcePtr->MarkSpecialInitializationAsCompleted();
		}

		D3D12_RESOURCE_STATES GPUResourceStateBarrierMerger::GetCurrentResourceState() const
		{
			return mResourcePtr->GetCurrentResourceState();
		}
	}
}