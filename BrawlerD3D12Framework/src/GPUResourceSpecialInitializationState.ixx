module;
#include <utility>
#include <cassert>

export module Brawler.D3D12.GPUResourceSpecialInitializationState;
import Brawler.D3D12.I_GPUResourceStateTrackerState;
import Brawler.D3D12.GPUResourceStateManagement;
import Brawler.D3D12.ResourceStateZone;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceLifetimeType;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceSpecialInitializationState : public I_GPUResourceStateTrackerState<GPUResourceSpecialInitializationState>
		{
		public:
			GPUResourceSpecialInitializationState() = default;

			bool ProcessResourceStateZone(const ResourceStateZone& stateZone);
			void OnStateDecayBarrier();
		};
	}
}

// ------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		bool GPUResourceSpecialInitializationState::ProcessResourceStateZone(const ResourceStateZone& stateZone)
		{
			assert(GetTrackedGPUResource().RequiresSpecialInitialization());
			
			// This ResourceStateZone can be used to perform special initialization of an
			// I_GPUResource iff the following conditions hold:
			//
			//   - The ResourceStateZone refers to a RenderPass which is executed on the
			//     DIRECT queue.
			//
			//   - The resource is a persistent resource, OR the ResourceStateZone is not
			//     null. (The latter refers to the fact that it is pointless to initialize
			//     an aliased resource until it needs to be used, since an aliasing barrier
			//     requires another special initialization.)
			const GPUResourceLifetimeType lifetimeType = GetTrackedGPUResource().GetGPUResourceLifetimeType();
			const bool canStateZoneInitializeResource = (stateZone.QueueType == GPUCommandQueueType::DIRECT && (lifetimeType == GPUResourceLifetimeType::PERSISTENT || !stateZone.IsNull()));

			// Move on to the next ResourceStateZone if we cannot use stateZone to perform
			// special initialization.
			if (!canStateZoneInitializeResource)
				return true;

			// Side Note: As an implementation detail, you might be wondering why we do not
			// check the resource's state before issuing a RESOURCE_INITIALIZATION event.
			// As it turns out, we never actually need to do this based on the restrictions
			// which we have put in place. Here are the details:
			//
			//   - Persistent resources are never aliased, so they would only ever need
			//     special initialization on their first use ever. We guarantee that these
			//     resources start out in their required state.
			//
			//   - Transient resources also start in the required state. In addition, we
			//     guarantee that a resource which is aliased in has never been used before;
			//     this makes the aliasing barrier case equivalent to the first use case.

			// Otherwise, issue a ResourceInitializationEvent and switch to the
			// ResourceBarrierTypeSelector state. We'll need to let it process stateZone,
			// so we return false to prevent the GPUResourceStateTracker from moving to
			// the next ResourceStateZone.
			GPUResourceEvent initializationEvent{
				.GPUResource{&(GetTrackedGPUResource())},
				.Event{ResourceInitializationEvent{}},
				.EventID{GPUResourceEventID::RESOURCE_INITIALIZATION}
			};

			AddGPUResourceEventForResourceStateZone(stateZone, std::move(initializationEvent));
			GetTrackedGPUResource().MarkSpecialInitializationAsCompleted();

			// Now that the special initialization has been completed, we can begin actually
			// transitioning resources.
			RequestTrackerStateChange(GPUResourceStateTrackerStateID::BARRIER_TYPE_SELECTOR);

			return false;
		}

		void GPUResourceSpecialInitializationState::OnStateDecayBarrier()
		{}
	}
}