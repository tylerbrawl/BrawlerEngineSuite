module;
#include <variant>
#include <optional>
#include <array>
#include <ranges>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceStateBarrierMerger;
import Util.D3D12;

namespace Brawler
{
	namespace D3D12
	{
		GPUResourceStateBarrierMerger::GPUResourceStateBarrierMerger(I_GPUResource& resource) :
			mStateContainer(resource),
			mEventManager(),
			mBeginBarrierPassArr(),
			mEndBarrierPass(),
			mWasLastStateForUAV(false)
		{}

		I_GPUResource& GPUResourceStateBarrierMerger::GetGPUResource()
		{
			return mStateContainer.GetGPUResource();
		}
		
		bool GPUResourceStateBarrierMerger::DoImplicitReadStateTransitionsAllowStateDecay() const
		{
			return mStateContainer.DoImplicitReadStateTransitionsAllowStateDecay();
		}

		void GPUResourceStateBarrierMerger::DecayResourceState()
		{
			CommitExistingExplicitResourceTransition();
			EraseBarrierPasses();

			mStateContainer.DecayResourceState();
		}

		GPUResourceEventManager GPUResourceStateBarrierMerger::FinalizeStateTracking()
		{
			CommitExistingExplicitResourceTransition();
			mStateContainer.UpdateGPUResourceState();

			return std::move(mEventManager);
		}

		void GPUResourceStateBarrierMerger::AddPotentialBeginBarrierRenderPass(GPUResourceStateBarrierMerger::RenderPassVariant passVariant)
		{
			const I_GPUResource& resource{ mStateContainer.GetGPUResource() };

			if (mEndBarrierPass.has_value() || resource.RequiresSpecialInitialization())
				return;
			
			// Try to add the first of each type of queue.
			for (auto& existingVariant : mBeginBarrierPassArr)
			{
				if (!existingVariant.has_value())
				{
					existingVariant = std::move(passVariant);
					break;
				}

				else if (existingVariant->index() == passVariant.index())
					return;
			}
		}

		void GPUResourceStateBarrierMerger::CommitExistingExplicitResourceTransition()
		{
			if (!mStateContainer.HasExplicitStateTransition())
				return;

			I_GPUResource& resource{ mStateContainer.GetGPUResource() };
			
			Util::D3D12::ResourceTransitionCheckInfo transitionCheckInfo{
				.QueueType{},
				.BeforeState = mStateContainer.GetBeforeState(),
				.AfterState = mStateContainer.GetAfterState()
			};

			bool shouldSubmitTransitionBarrier = false;

			if (transitionCheckInfo.BeforeState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON || transitionCheckInfo.AfterState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON)
				shouldSubmitTransitionBarrier = (transitionCheckInfo.BeforeState != transitionCheckInfo.AfterState);
			else
				shouldSubmitTransitionBarrier = ((transitionCheckInfo.BeforeState & transitionCheckInfo.AfterState) != transitionCheckInfo.AfterState);

			if (shouldSubmitTransitionBarrier)
			{
				bool usingSplitBarrier = false;

				static constexpr bool ENABLE_SPLIT_BARRIERS = true;

				if constexpr (ENABLE_SPLIT_BARRIERS)
				{
					for (const auto& beginBarrierPassVariant : mBeginBarrierPassArr | std::views::filter([this] (const std::optional<RenderPassVariant>& passVariant) { return passVariant.has_value(); }))
					{
						transitionCheckInfo.QueueType = static_cast<GPUCommandQueueType>(beginBarrierPassVariant->index());

						if (Util::D3D12::CanQueuePerformResourceTransition(transitionCheckInfo))
						{
							std::visit([this, &transitionCheckInfo] (const auto renderPassPtr)
							{
								mEventManager.AddGPUResourceEvent(*renderPassPtr, GPUResourceEvent{
									.GPUResource{ std::addressof(mStateContainer.GetGPUResource()) },
									.Event{ ResourceTransitionEvent{
										.BeforeState{ transitionCheckInfo.BeforeState },
										.AfterState{ transitionCheckInfo.AfterState },
										.Flags{ D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY }
									} },
									.EventID = GPUResourceEventID::RESOURCE_TRANSITION
									});
							}, *beginBarrierPassVariant);

							usingSplitBarrier = true;
							break;
						}
					}
				}

				assert(mEndBarrierPass.has_value());

				std::visit([this, &transitionCheckInfo, usingSplitBarrier] (const auto renderPassPtr)
				{
					mEventManager.AddGPUResourceEvent(*renderPassPtr, GPUResourceEvent{
						.GPUResource{ std::addressof(mStateContainer.GetGPUResource()) },
						.Event{ResourceTransitionEvent{
							.BeforeState{ transitionCheckInfo.BeforeState },
							.AfterState{ transitionCheckInfo.AfterState },
							.Flags{ (usingSplitBarrier ? D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_END_ONLY : D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE) }
						} },
						.EventID = GPUResourceEventID::RESOURCE_TRANSITION
						});
				}, *mEndBarrierPass);

				mStateContainer.SwapStates();
			}

			EraseBarrierPasses();
		}

		void GPUResourceStateBarrierMerger::EraseBarrierPasses()
		{
			for (auto& beginPassVariant : mBeginBarrierPassArr)
				beginPassVariant.reset();

			mEndBarrierPass.reset();
		}
	}
}