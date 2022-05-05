module;
#include <span>
#include "DxDef.h"

module Brawler.D3D12.UAVCounterSubAllocation;

namespace Brawler
{
	namespace D3D12
	{
		std::size_t UAVCounterSubAllocation::GetSubAllocationSize() const
		{
			return sizeof(std::uint32_t);
		}

		std::size_t UAVCounterSubAllocation::GetRequiredDataPlacementAlignment() const
		{
			// Fun Fact: The MSDN has conflicting information on the alignment of UAV counters!
			//
			//   - At https://docs.microsoft.com/en-us/windows/win32/direct3d12/uav-counters#using-uav-counters,
			//     it says that the alignment (CounterOffsetInBytes) must be a multiple of 4 bytes.
			//
			//   - At https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createunorderedaccessview#parameters,
			//     it says that the alignment (CounterOffsetInBytes) must be a multiple of
			//     D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT (4,096) bytes.
			//
			// I'm going to assume that the latter value is correct, since 4,096 is a multiple of 4.
			// If we can use a 4-byte alignment, however, then we could potentially be saving a lot
			// of memory!

			return D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
		}

		void UAVCounterSubAllocation::SetCounterValue(const std::uint32_t counterValue) const
		{
			const std::span<const std::uint32_t> srcDataSpan{ std::addressof(counterValue), 1 };
			WriteToBuffer(srcDataSpan, 0);
		}

		std::uint32_t UAVCounterSubAllocation::GetCounterValue() const
		{
			std::uint32_t counterValue = 0;

			{
				const std::span<std::uint32_t> destDataSpan{ std::addressof(counterValue), 1 };
				ReadFromBuffer(destDataSpan, 0);
			}

			return counterValue;
		}
	}
}