module;
#include <vector>
#include <span>
#include "DxDef.h"

module Brawler.D3D12.GPUBarrierGroup;
import Brawler.D3D12.I_GPUResource;

namespace Brawler
{
	namespace D3D12
	{
		void GPUBarrierGroup::AddImmediateResourceTransition(const GPUResourceWriteHandle hResource, const D3D12_RESOURCE_STATES desiredState)
		{
			mBarrierArr.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
				&(hResource->GetD3D12Resource()),
				hResource->GetCurrentResourceState(),
				desiredState,
				D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE
			));

			// Update the resource's tracked state on the CPU side.
			hResource->SetCurrentResourceState(desiredState);
		}

		GPUSplitBarrierToken GPUBarrierGroup::BeginSplitResourceTransition(const GPUResourceWriteHandle hResource, const D3D12_RESOURCE_STATES desiredState)
		{
			mBarrierArr.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
				&(hResource->GetD3D12Resource()),
				hResource->GetCurrentResourceState(),
				desiredState,
				D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY
			));

			// We don't want to update the resource's tracked state on the CPU side until we actually finish
			// the split resource transition.

			return GPUSplitBarrierToken{ *hResource, desiredState };
		}

		void GPUBarrierGroup::EndSplitResourceTransition(GPUSplitBarrierToken&& splitBarrierToken)
		{
			I_GPUResource& resource{ *(splitBarrierToken.GetBarrierInfo().ResourcePtr) };
			
			mBarrierArr.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
				&(resource.GetD3D12Resource()),
				resource.GetCurrentResourceState(),
				splitBarrierToken.GetBarrierInfo().DesiredState,
				D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_END_ONLY
			));

			// With the split barrier finished, we can now finish the transition on the CPU side.
			resource.SetCurrentResourceState(splitBarrierToken.GetBarrierInfo().DesiredState);

			// In Debug builds, we reset splitBarrierToken to ensure that it is not used again.
#ifdef _DEBUG
			splitBarrierToken = GPUSplitBarrierToken{};
#endif // _DEBUG
		}

		void GPUBarrierGroup::AddUAVBarrier(const GPUResourceWriteHandle hResource)
		{
			mBarrierArr.push_back(CD3DX12_RESOURCE_BARRIER::UAV(&(hResource->GetD3D12Resource())));
		}

		std::span<const CD3DX12_RESOURCE_BARRIER> GPUBarrierGroup::GetResourceBarriers() const
		{
			return std::span<const CD3DX12_RESOURCE_BARRIER>{ mBarrierArr };
		}
	}
}