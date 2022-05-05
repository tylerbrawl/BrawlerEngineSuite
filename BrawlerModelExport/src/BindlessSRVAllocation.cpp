module;
#include <cstdint>
#include <optional>
#include <cassert>

module Brawler.D3D12.GPUResourceDescriptors.BindlessSRVAllocation;
import Brawler.D3D12.GPUResourceDescriptors.GPUResourceDescriptorHeap;
import Util.Engine;

namespace Brawler
{
	namespace D3D12
	{
		BindlessSRVAllocation::BindlessSRVAllocation(const std::uint32_t allocationIndex) :
			mAllocationIndex(allocationIndex)
		{}

		BindlessSRVAllocation::~BindlessSRVAllocation()
		{
			ReClaimAllocation();
		}

		BindlessSRVAllocation::BindlessSRVAllocation(BindlessSRVAllocation&& rhs) noexcept :
			mAllocationIndex(std::move(rhs.mAllocationIndex))
		{
			rhs.ResetAllocationIndex();
		}

		BindlessSRVAllocation& BindlessSRVAllocation::operator=(BindlessSRVAllocation&& rhs) noexcept
		{
			ReClaimAllocation();

			mAllocationIndex = rhs.mAllocationIndex;
			rhs.ResetAllocationIndex();

			return *this;
		}

		std::uint32_t BindlessSRVAllocation::GetBindlessSRVIndex() const
		{
			assert(mAllocationIndex.has_value() && "ERROR: An attempt was made to get the bindless SRV index for a BindlessSRVAllocation, but it did not have a valid allocation index!");
			return *mAllocationIndex;
		}

		void BindlessSRVAllocation::ReClaimAllocation()
		{
			if (mAllocationIndex.has_value())
				Util::Engine::GetGPUResourceDescriptorHeap().ReClaimBindlessSRV(*this);
		}

		void BindlessSRVAllocation::ResetAllocationIndex()
		{
			mAllocationIndex.reset();
		}
	}
}