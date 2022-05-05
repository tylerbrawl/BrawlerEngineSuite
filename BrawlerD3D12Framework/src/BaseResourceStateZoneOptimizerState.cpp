module;

module Brawler.D3D12.BaseResourceStateZoneOptimizerState;
import Brawler.D3D12.ResourceStateZoneOptimizer;

namespace Brawler
{
	namespace D3D12
	{
		BaseResourceStateZoneOptimizerState::BaseResourceStateZoneOptimizerState(ResourceStateZoneOptimizer& optimizer) :
			mOptimizerPtr(&optimizer)
		{}

		void BaseResourceStateZoneOptimizerState::RequestResourceStateZoneDeletion(ResourceStateZone& stateZone)
		{
			mOptimizerPtr->AddResourceStateZoneForDeletion(stateZone);
		}

		const I_GPUResource& BaseResourceStateZoneOptimizerState::GetGPUResource() const
		{
			return mOptimizerPtr->GetGPUResource();
		}

		void BaseResourceStateZoneOptimizerState::SendOptimizerStateChangeRequest(const ResourceStateZoneOptimizerStateID stateID, ResourceStateZone* stateZonePtr)
		{
			mOptimizerPtr->RequestOptimizerStateChange(ResourceStateZoneOptimizer::StateChangeParams{
				.StateID = stateID,
				.StateZone = stateZonePtr
			});
		}
	}
}