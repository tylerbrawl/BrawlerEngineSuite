module;
#include <utility>
#include <cassert>

module Brawler.D3D12.BaseGPUResourceStateTrackerState;
import Brawler.D3D12.GPUResourceStateTracker;

namespace Brawler
{
	namespace D3D12
	{
		void BaseGPUResourceStateTrackerState::SetTrackedGPUResource(I_GPUResource& resource)
		{
			mResourcePtr = &resource;
		}

		void BaseGPUResourceStateTrackerState::SetGPUResourceStateTracker(GPUResourceStateTracker& stateTracker)
		{
			mStateTrackerPtr = &stateTracker;
		}

		I_GPUResource& BaseGPUResourceStateTrackerState::GetTrackedGPUResource()
		{
			assert(mResourcePtr != nullptr);
			return *mResourcePtr;
		}

		void BaseGPUResourceStateTrackerState::RequestTrackerStateChange(const GPUResourceStateTrackerStateID stateID)
		{
			assert(mStateTrackerPtr != nullptr);
			mStateTrackerPtr->RequestTrackerStateChange(stateID);
		}

		void BaseGPUResourceStateTrackerState::AddGPUResourceEventForResourceStateZone(const ResourceStateZone& stateZone, GPUResourceEvent&& resourceEvent)
		{
			assert(mStateTrackerPtr != nullptr);
			mStateTrackerPtr->AddGPUResourceEventForResourceStateZone(stateZone, std::move(resourceEvent));
		}
	}
}