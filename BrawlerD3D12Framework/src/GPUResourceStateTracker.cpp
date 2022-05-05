module;
#include <variant>
#include <cassert>
#include <optional>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceStateTracker;
import Brawler.D3D12.ResourceStateZone;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceStateManagement;
import Brawler.D3D12.GPUResourceSpecialInitializationState;
import Brawler.D3D12.GPUResourceBarrierTypeSelectorState;
import Brawler.D3D12.ImmediateGPUResourceBarrierState;
import Brawler.D3D12.SplitGPUResourceBarrierState;
import Brawler.D3D12.GPUResourceLifetimeType;

namespace Brawler
{
	namespace D3D12
	{
		GPUResourceStateTracker::GPUResourceStateTracker(I_GPUResource& resource) :
			mResourcePtr(&resource),
			mEventManager(),
			mCurrState(),
			mNextStateID()
		{
			if (mResourcePtr->RequiresSpecialInitialization())
				mCurrState = GPUResourceSpecialInitializationState{};
			else
				mCurrState = GPUResourceBarrierTypeSelectorState{};

			mCurrState.AccessData([this]<typename StateTrackerState_T>(I_GPUResourceStateTrackerState<StateTrackerState_T>&state)
			{
				state.SetTrackedGPUResource(*mResourcePtr);
				state.SetGPUResourceStateTracker(*this);
			});
		}

		void GPUResourceStateTracker::AddNextResourceStateZone(const ResourceStateZone& stateZone)
		{
			// Upon further inspection, it becomes apparent that we cannot simply perform
			// special resource initialization on a resource during the first null ResourceStateZone
			// on the DIRECT queue. The reason for this is due to aliasing.
			//
			// Specifically, if we try to initialize two or more resources which belong to the
			// same memory region in the same call to ExecuteCommandLists(), then we will need
			// an aliasing barrier. Suppose that the user wants to use Resource A, which shares
			// a memory region with Resource B.
			//
			// If we try to initialize every resource as early as possible, then we get the 
			// following:
			//
			// Initialize Resource A > Aliasing Barrier: A/B > Initialize Resource B >
			// Aliasing Barrier: B/A > Initialize Resource A
			//
			// Not only do we need two aliasing barriers (one to initialize Resource B and one to
			// go back to Resource A for the user's RenderPass), but we also have to initialize
			// Resource A a second time! Clearly, this is not optimal.
			//
			// I suppose that the best solution would technically find a GPUExecutionModule in
			// which none of the aliased resources are being used and initialize one resource
			// there, but this would add an insufferable amount of complexity for little actual
			// benefit in practice. So, we'll just postpone special initialization until the
			// first non-null ResourceStateZone, if it is required.
			//
			// ...On second thought, we actually *CAN* do this sometimes. Specifically, since
			// we do not alias persistent resources, we can indeed initialize these as soon as
			// possible.

			bool processResult = false;
			while (!processResult)
			{
				processResult = mCurrState.AccessData([&stateZone]<typename StateTrackerState_T>(StateTrackerState_T& state)
				{
					return state.ProcessResourceStateZone(stateZone);
				});
				ChangeTrackerState();
			}
		}

		void GPUResourceStateTracker::OnStateDecayBarrier()
		{
			mResourcePtr->SetCurrentResourceState(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
			
			mCurrState.AccessData([]<typename StateTrackerState_T>(StateTrackerState_T& state)
			{
				state.OnStateDecayBarrier();
			});
			ChangeTrackerState();
		}

		GPUResourceEventManager GPUResourceStateTracker::FinalizeStateTracking()
		{
			return std::move(mEventManager);
		}

		void GPUResourceStateTracker::RequestTrackerStateChange(const GPUResourceStateTrackerStateID stateID)
		{
			mNextStateID = stateID;
		}

		void GPUResourceStateTracker::AddGPUResourceEventForResourceStateZone(const ResourceStateZone& stateZone, GPUResourceEvent&& resourceEvent)
		{
			switch (stateZone.QueueType)
			{
			case GPUCommandQueueType::DIRECT:
			{
				mEventManager.AddGPUResourceEvent(*(std::get<const I_RenderPass<GPUCommandQueueType::DIRECT>*>(stateZone.EntranceRenderPass)), std::move(resourceEvent));
				return;
			}

			case GPUCommandQueueType::COMPUTE:
			{
				mEventManager.AddGPUResourceEvent(*(std::get<const I_RenderPass<GPUCommandQueueType::COMPUTE>*>(stateZone.EntranceRenderPass)), std::move(resourceEvent));
				return;
			}

			case GPUCommandQueueType::COPY:
			{
				mEventManager.AddGPUResourceEvent(*(std::get<const I_RenderPass<GPUCommandQueueType::COPY>*>(stateZone.EntranceRenderPass)), std::move(resourceEvent));
				return;
			}

			default:
			{
				__assume(false);
				break;
			}
			}
		}

		void GPUResourceStateTracker::ChangeTrackerState()
		{
			if (mNextStateID.has_value())
			{
				switch (*mNextStateID)
				{
				case GPUResourceStateTrackerStateID::GPU_RESOURCE_SPECIAL_INITIALIZATION:
				{
					mCurrState = GPUResourceSpecialInitializationState{};
					break;
				}

				case GPUResourceStateTrackerStateID::BARRIER_TYPE_SELECTOR:
				{
					mCurrState = GPUResourceBarrierTypeSelectorState{};
					break;
				}

				case GPUResourceStateTrackerStateID::IMMEDIATE_BARRIER:
				{
					mCurrState = ImmediateGPUResourceBarrierState{};
					break;
				}

				case GPUResourceStateTrackerStateID::SPLIT_BARRIER:
				{
					mCurrState = SplitGPUResourceBarrierState{};
					break;
				}

				default:
				{
					assert(false);
					std::unreachable();

					break;
				}
				}

				mCurrState.AccessData([this]<typename StateTrackerState_T>(StateTrackerState_T & state)
				{
					state.SetTrackedGPUResource(*mResourcePtr);
					state.SetGPUResourceStateTracker(*this);
				});

				mNextStateID.reset();
			}
		}
	}
}