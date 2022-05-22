module;
#include <optional>
#include <cassert>
#include <span>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceStateTracker;
import Brawler.D3D12.GPUResourceStateTrackingContext;
import Brawler.D3D12.ExplicitBarrierGPUResourceStateTrackerState;
import Brawler.D3D12.ImplicitBarrierGPUResourceStateTrackerState;

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void GPUResourceStateTracker::TrackRenderPass(const GPUExecutionModule& executionModule, const I_RenderPass<QueueType>& renderPass)
		{
			const GPUResourceStateTrackingContext context{
				.GPUResource{ *mResourcePtr },
				.EventManager{ mEventManager },
				.BarrierMerger{ mBarrierMerger },
				.ExecutionModule{ executionModule }
			};

			const std::optional<GPUResourceStateTrackerStateID> nextStateID{ mStateAdapter.AccessData([&renderPass, &context]<typename StateType>(StateType& state)
			{
				state.TrackRenderPass(renderPass, context);

				return state.GetNextStateID();
			}) };

			if (nextStateID.has_value())
				ChangeState(*nextStateID);
		}

		GPUResourceStateTracker::GPUResourceStateTracker(I_GPUResource& trackedResource) :
			mResourcePtr(&trackedResource),
			mEventManager(),
			mBarrierMerger(*mResourcePtr, mEventManager),
			mStateAdapter()
		{
			if (mResourcePtr->GetCurrentResourceState() == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON)
				mStateAdapter = ImplicitBarrierGPUResourceStateTrackerState{};
			else
				mStateAdapter = ExplicitBarrierGPUResourceStateTrackerState{};
		}

		void GPUResourceStateTracker::TrackGPUExecutionModule(const GPUExecutionModule& executionModule)
		{
			const auto trackRenderPassesLambda = [this]<GPUCommandQueueType QueueType>(const GPUExecutionModule& executionModule)
			{
				// Most resources won't be used in a lot of render passes. By filtering out
				// GPUExecutionModules which do not use the given resource, we can drastically
				// improve performance.
				const std::span<const std::unique_ptr<I_RenderPass<QueueType>>> renderPassSpan{ executionModule.GetRenderPassSpan<QueueType>() };

				if (executionModule.IsResourceUsedInQueue<QueueType>(*mResourcePtr)) [[unlikely]]
				{
					for (const auto& renderPassPtr : renderPassSpan)
						TrackRenderPass(executionModule, *renderPassPtr);
				}
				else if(!renderPassSpan.empty())
				{
					// Of course, even if a GPUExecutionModule's render passes are not using
					// a resource, we may still be able to use them for the sake of split barriers.
					mBarrierMerger.AddPotentialSplitBarrierBeginRenderPass(*(renderPassSpan[0]));
				}
			};

			auto directSpan{ executionModule.GetRenderPassSpan<GPUCommandQueueType::DIRECT>() };
			auto computeSpan{ executionModule.GetRenderPassSpan<GPUCommandQueueType::COMPUTE>() };
			auto copySpan{ executionModule.GetRenderPassSpan<GPUCommandQueueType::COPY>() };

			trackRenderPassesLambda.operator() < GPUCommandQueueType::DIRECT > (executionModule);
			trackRenderPassesLambda.operator() < GPUCommandQueueType::COMPUTE > (executionModule);
			trackRenderPassesLambda.operator() < GPUCommandQueueType::COPY > (executionModule);

			const GPUResourceStateTrackingContext context{
				.GPUResource{ *mResourcePtr },
				.EventManager{ mEventManager },
				.BarrierMerger{ mBarrierMerger },
				.ExecutionModule{ executionModule }
			};

			if (mStateAdapter.AccessData([&context]<typename StateType>(const StateType& state)
			{
				return state.WillResourceStateDecay(context);
			}))
			{
				mBarrierMerger.ErasePotentialSplitBarrierBeginRenderPasses();
				mResourcePtr->SetCurrentResourceState(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
				mStateAdapter = ImplicitBarrierGPUResourceStateTrackerState{};
			}
		}

		GPUResourceEventManager GPUResourceStateTracker::FinalizeStateTracking()
		{
			mBarrierMerger.CommitExistingExplicitStateTransition();

			return std::move(mEventManager);
		}

		void GPUResourceStateTracker::ChangeState(const GPUResourceStateTrackerStateID stateID)
		{
			switch (stateID)
			{
			case GPUResourceStateTrackerStateID::EXPLICIT_BARRIER:
			{
				mStateAdapter = ExplicitBarrierGPUResourceStateTrackerState{};
				break;
			}

			case GPUResourceStateTrackerStateID::IMPLICIT_BARRIER:
			{
				mStateAdapter = ImplicitBarrierGPUResourceStateTrackerState{};
				break;
			}

			default:
			{
				assert(false);
				std::unreachable();

				break;
			}
			}
		}
	}
}