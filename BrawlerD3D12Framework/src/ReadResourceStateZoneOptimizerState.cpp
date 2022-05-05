module;
#include <vector>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.ReadResourceStateZoneOptimizerState;
import Brawler.D3D12.ResourceStateZoneOptimizerStateID;
import Brawler.D3D12.GPUResourceStateManagement;
import Util.D3D12;
import Brawler.D3D12.ResourceStateZone;

namespace Brawler
{
	namespace D3D12
	{
		ReadResourceStateZoneOptimizerState::ReadResourceStateZoneOptimizerState(ResourceStateZone& stateZoneToOptimize, ResourceStateZoneOptimizer& optimizer) :
			I_ResourceStateZoneOptimizerState<ReadResourceStateZoneOptimizerState>(optimizer),
			mStateZoneToOptimize(&stateZoneToOptimize),
			mPotentiallyDeleteableStateZoneArr()
		{}

		void ReadResourceStateZoneOptimizerState::ProcessResourceStateZone(ResourceStateZone& stateZone)
		{
			// We optimize read-only ResourceStateZones by combining them and deleting any
			// null ResourceStateZones in between them.

			if (stateZone.IsNull())
				mPotentiallyDeleteableStateZoneArr.push_back(&stateZone);
			else
			{
				const D3D12_RESOURCE_STATES stateZoneRequiredState = *(stateZone.RequiredState);
				const D3D12_RESOURCE_STATES proposedNewReadState = *(mStateZoneToOptimize->RequiredState) | stateZoneRequiredState;

				const bool isReadStateZone = (stateZoneRequiredState != D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON && Util::D3D12::IsValidReadState(proposedNewReadState));

				// We can only combine the read states if the queue of mStateZoneToOptimize
				// is capable of handling the new read state.
				const bool canQueueHandleTransition = Util::D3D12::CanQueuePerformResourceTransition(mStateZoneToOptimize->QueueType, proposedNewReadState);

				if (isReadStateZone && canQueueHandleTransition)
				{
					DeleteNullResourceStateZones();
					MergeReadResourceStateZone(stateZone);
				}
				else if (isReadStateZone)
					RequestOptimizerStateChange<ResourceStateZoneOptimizerStateID::READ_RESOURCE_STATE_ZONE>(stateZone);
				else
					RequestOptimizerStateChange<ResourceStateZoneOptimizerStateID::IGNORE_RESOURCE_STATE_ZONE>();
			}
		}

		void ReadResourceStateZoneOptimizerState::OnStateDecayBarrier()
		{
			// When a resource decays, it returns back to the D3D12_RESOURCE_STATE_COMMON
			// state. Therefore, even if the next state is a read-only ResourceStateZone,
			// we don't want to merge its read state into mStateZoneToOptimize. If we did
			// that, then mStateZoneToOptimize would contain read states which it does not
			// need, since the RenderPass which actually needed it in the new state could
			// have just used implicit state promotion.
			//
			// Using the minimal set of read states for a resource is an important goal
			// in the D3D12 API. If the next ResourceStateZone after a decay barrier is
			// indeed a read-only ResourceStateZone, then the ResourceStateOptimizer
			// will notice this and begin optimizing that zone, instead.
			//
			// In doing this, we ensure that we always use the minimal set of read states
			// in a read-only ResourceStateZone.
		}

		void ReadResourceStateZoneOptimizerState::DeleteNullResourceStateZones()
		{
			// Delete all of the null ResourceStateZones between mStateZoneToOptimize and
			// stateZone.
			for (const auto nullStateZone : mPotentiallyDeleteableStateZoneArr)
			{
				assert(nullStateZone->IsNull());
				RequestResourceStateZoneDeletion(*nullStateZone);
			}

			mPotentiallyDeleteableStateZoneArr.clear();
		}

		void ReadResourceStateZoneOptimizerState::MergeReadResourceStateZone(ResourceStateZone& stateZone)
		{
			assert(!stateZone.IsNull() && *(stateZone.RequiredState) != D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON && Util::D3D12::IsValidReadState(*(mStateZoneToOptimize->RequiredState) | *(stateZone.RequiredState)));

			// If we have just passed a state decay barrier and we come across a
			// read-only ResourceStateZone as the first non-null ResourceStateZone
			// immediately following said barrier, then we combine it with
			// mStateZoneToOptimize iff the resource is incapable of being implicitly
			// promoted to the state of this new ResourceStateZone.
			//
			// In doing it like this, we keep the number of combined read states at
			// a minimum.
			const D3D12_RESOURCE_STATES combinedState = *(mStateZoneToOptimize->RequiredState) | *(stateZone.RequiredState);

			if (stateZone.IsImplicitTransition || (mStateZoneToOptimize->IsImplicitTransition && !Util::D3D12::IsImplicitStateTransitionPossible(GetGPUResource(), combinedState)))
				RequestOptimizerStateChange<ResourceStateZoneOptimizerStateID::READ_RESOURCE_STATE_ZONE>(stateZone);
			else
			{
				*(mStateZoneToOptimize->RequiredState) = combinedState;

				// We should also request that stateZone be deleted, since mStateZoneToOptimize
				// now contains the required state specified by stateZone.
				RequestResourceStateZoneDeletion(stateZone);
			}
		}
	}
}