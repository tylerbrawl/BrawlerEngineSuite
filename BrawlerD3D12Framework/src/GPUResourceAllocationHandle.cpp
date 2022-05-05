module;
#include <cstdint>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceAllocationHandle;
import Brawler.D3D12.GPUResourceAllocation;

namespace Brawler
{
	namespace D3D12
	{
		GPUResourceAllocationHandle::GPUResourceAllocationHandle(GPUResourceAllocation& owningAllocation, const I_GPUResource& resource) :
			mOwningAllocationPtr(&owningAllocation),
			mResourcePtr(&resource)
		{}

		GPUResourceAllocationHandle::GPUResourceAllocationHandle(GPUResourceAllocationHandle&& rhs) noexcept :
			mOwningAllocationPtr(rhs.mOwningAllocationPtr),
			mResourcePtr(rhs.mResourcePtr)
		{
			rhs.mOwningAllocationPtr = nullptr;
			rhs.mResourcePtr = nullptr;
		}

		GPUResourceAllocationHandle::~GPUResourceAllocationHandle()
		{
			if (mOwningAllocationPtr != nullptr) [[likely]]
				mOwningAllocationPtr->RevokeGPUResourceAssignment(*mResourcePtr);
		}

		GPUResourceAllocationHandle& GPUResourceAllocationHandle::operator=(GPUResourceAllocationHandle&& rhs) noexcept
		{
			mOwningAllocationPtr = rhs.mOwningAllocationPtr;
			rhs.mOwningAllocationPtr = nullptr;

			mResourcePtr = rhs.mResourcePtr;
			rhs.mResourcePtr = nullptr;

			return *this;
		}

		Brawler::D3D12Heap& GPUResourceAllocationHandle::GetD3D12Heap() const
		{
			assert(mOwningAllocationPtr != nullptr);
			return mOwningAllocationPtr->GetD3D12Heap();
		}

		std::size_t GPUResourceAllocationHandle::GetHeapOffset() const
		{
			assert(mOwningAllocationPtr != nullptr);
			return mOwningAllocationPtr->GetHeapOffset();
		}
	}
}