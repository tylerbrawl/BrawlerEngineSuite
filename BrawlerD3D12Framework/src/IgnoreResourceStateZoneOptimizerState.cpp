module;
#include "DxDef.h"

module Brawler.D3D12.IgnoreResourceStateZoneOptimizerState;
import Brawler.D3D12.ResourceStateZone;
import Brawler.D3D12.ResourceStateZoneOptimizerStateID;
import Util.D3D12;

namespace Brawler
{
	namespace D3D12
	{
		IgnoreResourceStateZoneOptimizerState::IgnoreResourceStateZoneOptimizerState(ResourceStateZoneOptimizer& optimizer) :
			I_ResourceStateZoneOptimizerState<IgnoreResourceStateZoneOptimizerState>(optimizer)
		{}

		void IgnoreResourceStateZoneOptimizerState::ProcessResourceStateZone(ResourceStateZone& stateZone)
		{
			if (!stateZone.IsNull() && *(stateZone.RequiredState) != D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON && Util::D3D12::IsValidReadState(*(stateZone.RequiredState)))
				RequestOptimizerStateChange<ResourceStateZoneOptimizerStateID::READ_RESOURCE_STATE_ZONE>(stateZone);
		}

		void IgnoreResourceStateZoneOptimizerState::OnStateDecayBarrier()
		{}
	}
}