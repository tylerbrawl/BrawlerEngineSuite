module;

export module Brawler.D3D12.GPUResourceBarrierTypeSelectorState;
import Brawler.D3D12.I_GPUResourceStateTrackerState;
import Brawler.D3D12.GPUResourceStateTrackerStateID;
import Brawler.D3D12.ResourceStateZone;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceBarrierTypeSelectorState : public I_GPUResourceStateTrackerState<GPUResourceBarrierTypeSelectorState>
		{
		public:
			GPUResourceBarrierTypeSelectorState() = default;

			bool ProcessResourceStateZone(const ResourceStateZone& stateZone);
			void OnStateDecayBarrier();
		};
	}
}

// ---------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		bool GPUResourceBarrierTypeSelectorState::ProcessResourceStateZone(const ResourceStateZone& stateZone)
		{
			// If the provided ResourceStateZone is null, then we can try to use it for a split barrier.
			// Otherwise, we need to create an immediate barrier.
			if (stateZone.IsNull())
				RequestTrackerStateChange(GPUResourceStateTrackerStateID::SPLIT_BARRIER);
			else
				RequestTrackerStateChange(GPUResourceStateTrackerStateID::IMMEDIATE_BARRIER);

			// Now, we need to let either of these two states handle stateZone, so we return false to
			// prevent the GPUResourceStateTracker from moving to the next ResourceStateZone.
			return false;
		}

		void GPUResourceBarrierTypeSelectorState::OnStateDecayBarrier()
		{}
	}
}