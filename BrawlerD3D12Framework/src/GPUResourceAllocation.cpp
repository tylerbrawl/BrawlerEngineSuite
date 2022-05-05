module;
#include <cassert>
#include <mutex>
#include <vector>
#include <algorithm>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceAllocation;
import Brawler.ScopedSharedLock;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceAllocationHandle;
import Brawler.D3D12.TLSFMemoryBlock;
import Brawler.D3D12.GPUResourceUsageTracker;

namespace Brawler
{
	namespace D3D12
	{
		void GPUResourceAllocation::SetD3D12Heap(Brawler::D3D12Heap& d3dHeap)
		{
			mD3DHeapPtr = &d3dHeap;
		}

		void GPUResourceAllocation::AssignGPUResource(I_GPUResource& resource)
		{
			GPUResourceAllocationHandle hAllocation{ *this, resource };
			mResourceInfoArr.PushBack(std::make_unique<GPUResourceInfo>(&resource));

			resource.CreatePlacedD3D12Resource(std::move(hAllocation));
		}

		void GPUResourceAllocation::RevokeGPUResourceAssignment(const I_GPUResource& resource)
		{
			mResourceInfoArr.EraseIf([resourcePtr = &resource] (const std::unique_ptr<GPUResourceInfo>& resourceInfo)
			{
				std::scoped_lock<std::mutex> lock{ resourceInfo->CritSection };

				return (resourcePtr == resourceInfo->GPUResourcePtr);
			});
		}

		Brawler::D3D12Heap& GPUResourceAllocation::GetD3D12Heap() const
		{
			assert(mD3DHeapPtr != nullptr);
			return *mD3DHeapPtr;
		}

		std::size_t GPUResourceAllocation::GetHeapOffset() const
		{
			assert(mAllocatedBlockPtr != nullptr);
			return mAllocatedBlockPtr->GetHeapOffset();
		}
		
		TLSFMemoryBlock& GPUResourceAllocation::GetTLSFBlock()
		{
			assert(mAllocatedBlockPtr != nullptr);
			return *mAllocatedBlockPtr;
		}

		const TLSFMemoryBlock& GPUResourceAllocation::GetTLSFBlock() const
		{
			assert(mAllocatedBlockPtr != nullptr);
			return *mAllocatedBlockPtr;
		}

		bool GPUResourceAllocation::HasGPUResourceAssignments() const
		{
			return !mResourceInfoArr.Empty();
		}

		bool GPUResourceAllocation::CouldAllocatedResourcesBeEvicted() const
		{
			bool isEvictionPossible = true;

			mResourceInfoArr.ForEach([&isEvictionPossible] (const std::unique_ptr<GPUResourceInfo>& info)
			{
				std::scoped_lock<std::mutex> lock{ info->CritSection };

				if (!(info->GPUResourcePtr->GetUsageTracker().IsResourceSafeToEvict()))
					isEvictionPossible = false;
			});

			return isEvictionPossible;
		}

		float GPUResourceAllocation::GetMaximumGPUResourceUsageMetric() const
		{
			float usageMetric = 0.0f;

			mResourceInfoArr.ForEach([&usageMetric] (const std::unique_ptr<GPUResourceInfo>& info)
			{
				std::scoped_lock<std::mutex> lock{ info->CritSection };

				usageMetric = std::max<float>(usageMetric, info->GPUResourcePtr->GetUsageTracker().GetResourceUsageMetric());
			});

			return usageMetric;
		}

		void GPUResourceAllocation::SetTLSFBlock(TLSFMemoryBlock& block)
		{
			mAllocatedBlockPtr = &block;
		}

		void GPUResourceAllocation::ResetTLSFBlock()
		{
			mAllocatedBlockPtr = nullptr;
		}
	}
}