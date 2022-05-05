module;
#include <cassert>

export module Brawler.D3D12.BaseGPUResourceStateTrackerState;
import Brawler.D3D12.GPUResourceStateTrackerStateID;

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource;
		struct GPUResourceEvent;
		class GPUResourceStateTracker;
		struct ResourceStateZone;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class BaseGPUResourceStateTrackerState
		{
		protected:
			BaseGPUResourceStateTrackerState() = default;

		public:
			virtual ~BaseGPUResourceStateTrackerState() = default;

			void SetTrackedGPUResource(I_GPUResource& resource);
			void SetGPUResourceStateTracker(GPUResourceStateTracker& stateTracker);

		protected:
			I_GPUResource& GetTrackedGPUResource();
			void RequestTrackerStateChange(const GPUResourceStateTrackerStateID stateID);

			void AddGPUResourceEventForResourceStateZone(const ResourceStateZone& stateZone, GPUResourceEvent&& resourceEvent);

		private:
			I_GPUResource* mResourcePtr;
			GPUResourceStateTracker* mStateTrackerPtr;
		};
	}
}