module;
#include <vector>
#include <span>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.GPUSubResourceStateManager;

namespace Brawler
{
	namespace D3D12
	{
		GPUSubResourceStateManager::GPUSubResourceStateManager(const D3D12_RESOURCE_STATES initialState, const std::uint32_t subResourceCount) :
			mSubResourceStateArr()
		{
			mSubResourceStateArr.resize(subResourceCount);
			SetSubResourceState(initialState, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
		}

		void GPUSubResourceStateManager::SetSubResourceState(const D3D12_RESOURCE_STATES state, const std::uint32_t subResourceIndex)
		{
			if (subResourceIndex == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
			{
				for (auto& trackedState : mSubResourceStateArr)
					trackedState = state;
			}
			else
			{
				assert(subResourceIndex < mSubResourceStateArr.size() && "ERROR: An out-of-bounds sub-resource index was specified in a call to GPUSubResourceStateManager::SetSubResourceState()!");
				mSubResourceStateArr[subResourceIndex] = state;
			}
		}

		D3D12_RESOURCE_STATES GPUSubResourceStateManager::GetSubResourceState(const std::uint32_t subResourceIndex) const
		{
			assert(subResourceIndex < mSubResourceStateArr.size() && "ERROR: An out-of-bounds sub-resource index was specified in a call to GPUSubResourceStateManager::GetSubResourceState()!");
			return mSubResourceStateArr[subResourceIndex];
		}

		std::span<const D3D12_RESOURCE_STATES> GPUSubResourceStateManager::GetAllSubResourceStates() const
		{
			return std::span<const D3D12_RESOURCE_STATES>{ mSubResourceStateArr };
		}
	}
}