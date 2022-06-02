module;
#include <cassert>
#include <span>
#include <cstddef>
#include <memory>
#include "DxDef.h"

module Brawler.D3D12.I_BufferSubAllocation;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.BufferSubAllocationReservation;

namespace Brawler
{
	namespace D3D12
	{
		std::size_t I_BufferSubAllocation::GetOffsetFromBufferStart() const
		{
			assert(HasReservation());
			return mHReservation->GetOffsetFromBufferStart();
		}

		Brawler::D3D12Resource& I_BufferSubAllocation::GetD3D12Resource() const
		{
			assert(HasReservation());
			return mHReservation->GetBufferSubAllocationManager().GetBufferD3D12Resource();
		}

		BufferResource& I_BufferSubAllocation::GetBufferResource()
		{
			assert(HasReservation());
			return mHReservation->GetBufferSubAllocationManager().GetBufferResource();
		}

		const BufferResource& I_BufferSubAllocation::GetBufferResource() const
		{
			assert(HasReservation());
			return mHReservation->GetBufferSubAllocationManager().GetBufferResource();
		}

		D3D12_GPU_VIRTUAL_ADDRESS I_BufferSubAllocation::GetGPUVirtualAddress() const
		{
			assert(HasReservation());
			return (mHReservation->GetBufferSubAllocationManager().GetBufferGPUVirtualAddress() + mHReservation->GetOffsetFromBufferStart());
		}

		bool I_BufferSubAllocation::IsReservationCompatible(const BufferSubAllocationReservationHandle& hReservation) const
		{
			// A BufferSubAllocationReservation is compatible with this sub-allocation if it is both large
			// enough to store all of its relevant data and aligned to the required data placement alignment.

			return ((hReservation->GetOffsetFromBufferStart() % GetRequiredDataPlacementAlignment() == 0) && (hReservation->GetReservationSize() >= GetSubAllocationSize()));
		}

		void I_BufferSubAllocation::AssignReservation(BufferSubAllocationReservationHandle&& hReservation)
		{
			assert(IsReservationCompatible(hReservation) && "ERROR: An attempt was made to assign an incompatible BufferSubAllocationReservation to a (derived) BaseBufferSubAllocation object!");
			mHReservation = std::move(hReservation);

			OnReservationAssigned();
		}

		BufferSubAllocationReservationHandle I_BufferSubAllocation::RevokeReservation()
		{
			return std::move(mHReservation);
		}

		void I_BufferSubAllocation::OnReservationAssigned()
		{}

		bool I_BufferSubAllocation::HasReservation() const
		{
			return (mHReservation.IsReservationValid());
		}

		BufferSubAllocationManager& I_BufferSubAllocation::GetOwningManager() const
		{
			assert(HasReservation());
			return mHReservation->GetBufferSubAllocationManager();
		}

		void I_BufferSubAllocation::WriteToBufferIMPL(const std::span<const std::byte> srcDataByteSpan, const std::size_t subAllocationOffsetInBytes) const
		{
			GetOwningManager().WriteToBuffer(srcDataByteSpan, GetOffsetFromBufferStart() + subAllocationOffsetInBytes);
		}

		void I_BufferSubAllocation::ReadFromBufferIMPL(const std::span<std::byte> destDataByteSpan, const std::size_t subAllocationOffsetInBytes) const
		{
			GetOwningManager().ReadFromBuffer(destDataByteSpan, GetOffsetFromBufferStart() + subAllocationOffsetInBytes);
		}
	}
}