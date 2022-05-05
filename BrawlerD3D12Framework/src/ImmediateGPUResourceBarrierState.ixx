module;
#include <cassert>
#include <utility>

export module Brawler.D3D12.ImmediateGPUResourceBarrierState;
import Brawler.D3D12.I_GPUResourceStateTrackerState;
import Brawler.D3D12.GPUResourceStateTrackerStateID;
import Brawler.D3D12.GPUResourceStateManagement;
import Brawler.D3D12.ResourceStateZone;
import Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		class ImmediateGPUResourceBarrierState : public I_GPUResourceStateTrackerState<ImmediateGPUResourceBarrierState>
		{
		public:
			ImmediateGPUResourceBarrierState() = default;

			bool ProcessResourceStateZone(const ResourceStateZone& stateZone);
			void OnStateDecayBarrier();

		private:
			void AddImmediateTransitionEvent(const ResourceStateZone& stateZone);
		};
	}
}

// ----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		bool ImmediateGPUResourceBarrierState::ProcessResourceStateZone(const ResourceStateZone& stateZone)
		{
			// If we find a null ResourceStateZone, then switch over to the split barrier state
			// and allow it to process stateZone by returning false.
			if (stateZone.IsNull())
			{
				RequestTrackerStateChange(GPUResourceStateTrackerStateID::SPLIT_BARRIER);
				return false;
			}

			// Otherwise, since this is the immediate barrier state, we need to issue an explicit
			// immediate transition barrier.
			AddImmediateTransitionEvent(stateZone);

			// Move on to the next ResourceStateZone.
			return true;
		}

		void ImmediateGPUResourceBarrierState::OnStateDecayBarrier()
		{}

		void ImmediateGPUResourceBarrierState::AddImmediateTransitionEvent(const ResourceStateZone& stateZone)
		{
			assert(!stateZone.IsNull());

			const D3D12_RESOURCE_STATES currResourceState = GetTrackedGPUResource().GetCurrentResourceState();
			const D3D12_RESOURCE_STATES desiredState = *(stateZone.RequiredState);

			// Do not perform any barriers if the transition is implicit.
			if (stateZone.IsImplicitTransition)
			{
				GetTrackedGPUResource().SetCurrentResourceState(desiredState);
				return;
			}

			// Drop the transition if the resource's current state already contains the requested
			// state.

			if (desiredState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON && currResourceState == desiredState)
				return;

			if (desiredState != D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON && (currResourceState & desiredState) == desiredState)
			{
				// If both currResourceState and desiredState are D3D12_RESOURCE_STATE_UNORDERED_ACCESS, then we
				// issue a UAV barrier here.
				if (currResourceState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS && currResourceState == desiredState)
					AddGPUResourceEventForResourceStateZone(stateZone, GPUResourceEvent{
						.GPUResource{&(GetTrackedGPUResource())},
						.Event{UAVBarrierEvent{}},
						.EventID = GPUResourceEventID::UAV_BARRIER
					});

				return;
			}

			ResourceTransitionEvent transitionEvent{
				.BeforeState = currResourceState,
				.AfterState = desiredState,
				.Flags{}
			};

			GPUResourceEvent resourceEvent{
				.GPUResource{&(GetTrackedGPUResource())},
				.Event{std::move(transitionEvent)},
				.EventID{GPUResourceEventID::RESOURCE_TRANSITION}
			};

			AddGPUResourceEventForResourceStateZone(stateZone, std::move(resourceEvent));

			GetTrackedGPUResource().SetCurrentResourceState(desiredState);
		}
	}
}