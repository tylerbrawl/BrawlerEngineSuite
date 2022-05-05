module;
#include <cstddef>
#include <cassert>

module Brawler.D3D12.BufferSubAllocationReservation;
import Brawler.D3D12.TLSFMemoryBlock;
import Brawler.D3D12.BufferSubAllocationManager;

namespace Brawler
{
	namespace D3D12
	{
		BufferSubAllocationReservation::~BufferSubAllocationReservation()
		{
			ReturnReservation();
		}

		BufferSubAllocationReservation::BufferSubAllocationReservation(BufferSubAllocationReservation&& rhs) noexcept :
			mOwningManagerPtr(rhs.mOwningManagerPtr),
			mMemoryBlockPtr(rhs.mMemoryBlockPtr)
		{
			rhs.mOwningManagerPtr = nullptr;
			rhs.mMemoryBlockPtr = nullptr;
		}

		BufferSubAllocationReservation& BufferSubAllocationReservation::operator=(BufferSubAllocationReservation&& rhs) noexcept
		{
			ReturnReservation();

			mOwningManagerPtr = rhs.mOwningManagerPtr;
			rhs.mOwningManagerPtr = nullptr;

			mMemoryBlockPtr = rhs.mMemoryBlockPtr;
			rhs.mMemoryBlockPtr = nullptr;

			return *this;
		}

		BufferSubAllocationManager& BufferSubAllocationReservation::GetBufferSubAllocationManager() const
		{
			assert(mOwningManagerPtr != nullptr);
			return *mOwningManagerPtr;
		}

		std::size_t BufferSubAllocationReservation::GetReservationSize() const
		{
			assert(mMemoryBlockPtr != nullptr);
			return mMemoryBlockPtr->GetBlockSize();
		}

		std::size_t BufferSubAllocationReservation::GetOffsetFromBufferStart() const
		{
			assert(mMemoryBlockPtr != nullptr);
			return mMemoryBlockPtr->GetHeapOffset();
		}

		void BufferSubAllocationReservation::ReturnReservation()
		{
			if (mOwningManagerPtr != nullptr && mMemoryBlockPtr != nullptr) [[likely]]
			{
				mOwningManagerPtr->DeleteSubAllocation(*this);

				mOwningManagerPtr = nullptr;
				mMemoryBlockPtr = nullptr;
			}
		}

		void BufferSubAllocationReservation::SetOwningManager(BufferSubAllocationManager& owningManager)
		{
			mOwningManagerPtr = &owningManager;
		}

		TLSFMemoryBlock& BufferSubAllocationReservation::GetTLSFMemoryBlock() const
		{
			assert(mMemoryBlockPtr != nullptr);
			return *mMemoryBlockPtr;
		}

		void BufferSubAllocationReservation::SetTLSFMemoryBlock(TLSFMemoryBlock& memoryBlock)
		{
			mMemoryBlockPtr = &memoryBlock;
		}
	}
}