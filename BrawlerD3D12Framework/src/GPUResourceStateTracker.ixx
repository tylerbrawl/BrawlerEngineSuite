module;
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceStateTracker;
import Brawler.D3D12.GPUResourceStateManagement;
import Brawler.D3D12.GPUResourceStateTrackerStateID;
import Brawler.PolymorphicAdapter;
export import Brawler.GPUResourceStateTrackerStateTraits;
import Brawler.D3D12.I_GPUResourceStateTrackerState;

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceStateTracker
		{
		private:
			friend class BaseGPUResourceStateTrackerState;

		public:
			explicit GPUResourceStateTracker(I_GPUResource& resource);

			GPUResourceStateTracker(const GPUResourceStateTracker& rhs) = delete;
			GPUResourceStateTracker& operator=(const GPUResourceStateTracker& rhs) = delete;

			GPUResourceStateTracker(GPUResourceStateTracker&& rhs) noexcept = default;
			GPUResourceStateTracker& operator=(GPUResourceStateTracker&& rhs) noexcept = default;

			void AddNextResourceStateZone(const ResourceStateZone& stateZone);
			void OnStateDecayBarrier();

			GPUResourceEventManager FinalizeStateTracking();

		private:
			void RequestTrackerStateChange(const GPUResourceStateTrackerStateID stateID);

			void AddGPUResourceEventForResourceStateZone(const ResourceStateZone& stateZone, GPUResourceEvent&& resourceEvent);
			void ChangeTrackerState();

		private:
			I_GPUResource* mResourcePtr;
			GPUResourceEventManager mEventManager;
			Brawler::PolymorphicAdapter<I_GPUResourceStateTrackerState> mCurrState;
			std::optional<GPUResourceStateTrackerStateID> mNextStateID;
		};
	}
}