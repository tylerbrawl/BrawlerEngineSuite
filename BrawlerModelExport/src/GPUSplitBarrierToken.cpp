module;
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.GPUSplitBarrierToken;
import Brawler.D3D12.I_GPUResource;

namespace Brawler
{
	namespace D3D12
	{
		GPUSplitBarrierToken::GPUSplitBarrierToken(I_GPUResource& resource, const D3D12_RESOURCE_STATES desiredState) :
			mBarrierInfo(BarrierInfo{
				.ResourcePtr = &resource,
				.DesiredState = desiredState
			})
		{}

		const GPUSplitBarrierToken::BarrierInfo& GPUSplitBarrierToken::GetBarrierInfo() const
		{
			assert(mBarrierInfo.ResourcePtr != nullptr && "ERROR: A GPUSplitBarrierToken was never provided an I_GPUResource to transition!");

			return mBarrierInfo;
		}
	}
}