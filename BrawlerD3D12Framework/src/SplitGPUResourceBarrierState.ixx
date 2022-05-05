module;
#include <array>
#include <algorithm>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.SplitGPUResourceBarrierState;
import Brawler.D3D12.I_GPUResourceStateTrackerState;
import Brawler.D3D12.GPUResourceStateManagement;
import Brawler.D3D12.GPUResourceStateTrackerStateID;
import Util.D3D12;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.ResourceStateZone;

export namespace Brawler
{
	namespace D3D12
	{
		class SplitGPUResourceBarrierState : public I_GPUResourceStateTrackerState<SplitGPUResourceBarrierState>
		{
		public:
			SplitGPUResourceBarrierState() = default;

			bool ProcessResourceStateZone(const ResourceStateZone& stateZone);
			void OnStateDecayBarrier();

		private:
			void HandleNullResourceStateZone(const ResourceStateZone& stateZone);
			void PerformResourceStateTransition(const ResourceStateZone& endBarrierStateZone);

		private:
			std::array<const ResourceStateZone*, 3> mCandidateNullZoneArr;
		};
	}
}

// -----------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		bool SplitGPUResourceBarrierState::ProcessResourceStateZone(const ResourceStateZone& stateZone)
		{
			if (stateZone.IsNull())
				HandleNullResourceStateZone(stateZone);
			else
			{
				PerformResourceStateTransition(stateZone);

				// Request that we move back to the immediate barrier state.
				RequestTrackerStateChange(GPUResourceStateTrackerStateID::IMMEDIATE_BARRIER);
			}
			
			// We have finished handling this ResourceStateZone, so we let the GPUResourceStateTracker
			// move to the next one.
			return true;
		}

		void SplitGPUResourceBarrierState::OnStateDecayBarrier()
		{
			RequestTrackerStateChange(GPUResourceStateTrackerStateID::IMMEDIATE_BARRIER);
		}

		void SplitGPUResourceBarrierState::HandleNullResourceStateZone(const ResourceStateZone& stateZone)
		{
			// If we have a null ResourceStateZone, then we *may* be able to use it for a split barrier.
			// The problem is that not every queue is capable of performing every transition. So, we
			// want to choose the first null ResourceStateZone which is capable of performing the
			// transition.
			assert(stateZone.IsNull());

			auto findResult = std::ranges::find_if(mCandidateNullZoneArr, [&stateZone] (const ResourceStateZone* candidateStateZone) { return (candidateStateZone != nullptr && candidateStateZone->QueueType == stateZone.QueueType); });

			if (findResult == mCandidateNullZoneArr.end())
			{
				for (auto& stateZonePtr : mCandidateNullZoneArr)
				{
					if (stateZonePtr == nullptr)
					{
						stateZonePtr = &stateZone;
						break;
					}
				}
			}
		}

		void SplitGPUResourceBarrierState::PerformResourceStateTransition(const ResourceStateZone& endBarrierStateZone)
		{
			assert(!endBarrierStateZone.IsNull());

			const D3D12_RESOURCE_STATES currState = GetTrackedGPUResource().GetCurrentResourceState();
			const D3D12_RESOURCE_STATES newState = *(endBarrierStateZone.RequiredState);

			// Do not perform any barriers if the transition is implicit.
			if (endBarrierStateZone.IsImplicitTransition)
			{
				GetTrackedGPUResource().SetCurrentResourceState(newState);
				return;
			}

			// Drop the barrier if the before and after states are the same.

			if (currState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON && currState == newState)
				return;

			if (newState != D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON && (currState & newState) == newState)
			{
				// Issue a UAV barrier if both RenderPasses required the
				// D3D12_RESOURCE_STATE_UNORDERED_ACCESS state.
				if (currState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS && currState == newState)
					AddGPUResourceEventForResourceStateZone(endBarrierStateZone, GPUResourceEvent{
						.GPUResource{&(GetTrackedGPUResource())},
						.Event{UAVBarrierEvent{}},
						.EventID = GPUResourceEventID::UAV_BARRIER
					});

				return;
			}

			// Find the first null ResourceStateZone whose RenderPass is capable of handling
			// the resource transition.

			auto findResult = std::ranges::find_if(mCandidateNullZoneArr, [currState, newState](const ResourceStateZone* candidateStateZone)
			{
				if (candidateStateZone == nullptr)
					return false;

				const Util::D3D12::ResourceTransitionCheckInfo checkInfo{
					.QueueType = candidateStateZone->QueueType,
					.BeforeState = currState,
					.AfterState = newState
				};
				return Util::D3D12::CanQueuePerformResourceTransition(checkInfo);
			});

			D3D12_RESOURCE_BARRIER_FLAGS endBarrierFlags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			if (findResult != mCandidateNullZoneArr.end())
			{
				// If we found an eligible null ResourceStateZone, then we can give that the
				// begin barrier and endBarrierStateZone the end barrier.
				ResourceTransitionEvent beginTransitionEvent{
					.BeforeState = currState,
					.AfterState = newState,
					.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY
				};

				GPUResourceEvent beginResourceEvent{
					.GPUResource{&(GetTrackedGPUResource())},
					.Event{std::move(beginTransitionEvent)},
					.EventID = GPUResourceEventID::RESOURCE_TRANSITION
				};

				AddGPUResourceEventForResourceStateZone(*(*findResult), std::move(beginResourceEvent));

				endBarrierFlags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
			}

			ResourceTransitionEvent endTransitionEvent{
				.BeforeState = currState,
				.AfterState = newState,
				.Flags = endBarrierFlags
			};

			GPUResourceEvent endResourceEvent{
				.GPUResource{&(GetTrackedGPUResource())},
				.Event{std::move(endTransitionEvent)},
				.EventID = GPUResourceEventID::RESOURCE_TRANSITION
			};

			AddGPUResourceEventForResourceStateZone(endBarrierStateZone, std::move(endResourceEvent));

			GetTrackedGPUResource().SetCurrentResourceState(newState);
		}
	}
}