module;
#include <variant>
#include <optional>
#include <array>
#include <ranges>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.GPUSubResourceStateBarrierMerger;
import Util.D3D12;

namespace Brawler
{
	namespace D3D12
	{
		GPUSubResourceStateBarrierMerger::GPUSubResourceStateBarrierMerger(I_GPUResource& resource, const std::uint32_t subResourceIndex) :
			mStateContainer(resource, subResourceIndex),
			mEventCollection(),
			mBeginBarrierPassArr(),
			mEndBarrierPass(),
			mWasLastStateForUAV(false)
		{}

		I_GPUResource& GPUSubResourceStateBarrierMerger::GetGPUResource()
		{
			return mStateContainer.GetGPUResource();
		}

		std::uint32_t GPUSubResourceStateBarrierMerger::GetSubResourceIndex() const
		{
			return mStateContainer.GetSubResourceIndex();
		}
		
		bool GPUSubResourceStateBarrierMerger::DoImplicitReadStateTransitionsAllowStateDecay() const
		{
			return mStateContainer.DoImplicitReadStateTransitionsAllowStateDecay();
		}

		void GPUSubResourceStateBarrierMerger::DecayResourceState()
		{
			CommitExistingExplicitResourceTransition();
			EraseBarrierPasses();

			mStateContainer.DecayResourceState();
		}

		GPUResourceEventCollection GPUSubResourceStateBarrierMerger::FinalizeStateTracking()
		{
			CommitExistingExplicitResourceTransition();
			mStateContainer.UpdateGPUResourceState();

			return std::move(mEventCollection);
		}

		void GPUSubResourceStateBarrierMerger::AddPotentialBeginBarrierRenderPass(GPUSubResourceStateBarrierMerger::RenderPassVariant passVariant)
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

		bool GPUSubResourceStateBarrierMerger::CanUseAdditionalBeginBarrierRenderPasses() const
		{
			// We can still use additional render passes for begin barriers, unless we already have
			// one in the DIRECT queue. This is because the DIRECT queue is capable of performing any
			// resource transition, but the same cannot be said for other queues.

			for (const auto& existingVariant : mBeginBarrierPassArr)
			{
				if (existingVariant.has_value() && existingVariant->index() == 0)
					return false;
			}

			return true;
		}

		void GPUSubResourceStateBarrierMerger::CommitExistingExplicitResourceTransition()
		{
			if (!mStateContainer.HasExplicitStateTransition())
			{
				mStateContainer.SwapStates();
				EraseBarrierPasses();

				return;
			}

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

				for (const auto& beginBarrierPassVariant : mBeginBarrierPassArr)
				{
					if (!beginBarrierPassVariant.has_value())
						break;
					
					transitionCheckInfo.QueueType = static_cast<GPUCommandQueueType>(beginBarrierPassVariant->index());

					if (Util::D3D12::CanQueuePerformResourceTransition(transitionCheckInfo))
					{
						std::visit([this, &transitionCheckInfo] (const auto renderPassPtr)
						{
							mEventCollection.AddGPUResourceEvent(*renderPassPtr, GPUResourceEvent{
								.GPUResource{ std::addressof(mStateContainer.GetGPUResource()) },
								.Event{ ResourceTransitionEvent{
									.SubResourceIndex{ GetSubResourceIndex() },
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

				assert(mEndBarrierPass.has_value());

				std::visit([this, &transitionCheckInfo, usingSplitBarrier] (const auto renderPassPtr)
				{
					mEventCollection.AddGPUResourceEvent(*renderPassPtr, GPUResourceEvent{
						.GPUResource{ std::addressof(mStateContainer.GetGPUResource()) },
						.Event{ResourceTransitionEvent{
							.SubResourceIndex{ GetSubResourceIndex() },
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

		void GPUSubResourceStateBarrierMerger::EraseBarrierPasses()
		{
			for (auto& beginPassVariant : mBeginBarrierPassArr)
				beginPassVariant.reset();

			mEndBarrierPass.reset();
		}
	}
}