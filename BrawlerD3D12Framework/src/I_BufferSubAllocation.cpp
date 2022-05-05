module;
#include <cassert>
#include <span>
#include <cstddef>
#include <memory>
#include "DxDef.h"

module Brawler.D3D12.I_BufferSubAllocation;
import Brawler.D3D12.BufferSubAllocationManager;

namespace Brawler
{
	namespace D3D12
	{
		std::size_t I_BufferSubAllocation::GetOffsetFromBufferStart() const
		{
			assert(mReservation != nullptr);
			return mReservation->GetOffsetFromBufferStart();
		}

		Brawler::D3D12Resource& I_BufferSubAllocation::GetD3D12Resource() const
		{
			assert(mReservation != nullptr);
			return mReservation->GetBufferSubAllocationManager().GetBufferD3D12Resource();
		}

		const BufferResource& I_BufferSubAllocation::GetBufferResource() const
		{
			assert(mReservation != nullptr);
			return mReservation->GetBufferSubAllocationManager().GetBufferResource();
		}

		D3D12_GPU_VIRTUAL_ADDRESS I_BufferSubAllocation::GetGPUVirtualAddress() const
		{
			assert(mReservation != nullptr);
			return (mReservation->GetBufferSubAllocationManager().GetBufferGPUVirtualAddress() + mReservation->GetOffsetFromBufferStart());
		}

		bool I_BufferSubAllocation::IsReservationCompatible(const BufferSubAllocationReservation& reservation) const
		{
			// A BufferSubAllocationReservation is compatible with this sub-allocation if it is both large
			// enough to store all of its relevant data and aligned to the required data placement alignment.

			return ((reservation.GetOffsetFromBufferStart() % GetRequiredDataPlacementAlignment() == 0) && (reservation.GetReservationSize() >= GetSubAllocationSize()));
		}

		void I_BufferSubAllocation::AssignReservation(std::unique_ptr<BufferSubAllocationReservation>&& reservation)
		{
			assert(IsReservationCompatible(*reservation) && "ERROR: An attempt was made to assign an incompatible BufferSubAllocationReservation to a (derived) BaseBufferSubAllocation object!");
			mReservation = std::move(reservation);

			OnReservationAssigned();
		}

		std::unique_ptr<BufferSubAllocationReservation> I_BufferSubAllocation::RevokeReservation()
		{
			return std::move(mReservation);
		}

		void I_BufferSubAllocation::OnReservationAssigned()
		{}

		bool I_BufferSubAllocation::HasReservation() const
		{
			return (mReservation != nullptr);
		}

		BufferSubAllocationManager& I_BufferSubAllocation::GetOwningManager() const
		{
			assert(mReservation != nullptr);
			return mReservation->GetBufferSubAllocationManager();
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