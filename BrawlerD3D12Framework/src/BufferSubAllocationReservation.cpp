module;
#include <memory>
#include <atomic>
#include <cassert>

module Brawler.D3D12.BufferSubAllocationReservation;
import Brawler.D3D12.TLSFMemoryBlock;
import Brawler.D3D12.BufferSubAllocationManager;

namespace Brawler
{
	namespace D3D12
	{
		BufferSubAllocationReservation::BufferSubAllocationReservation() :
			mOwningManagerPtr(nullptr),
			mMemoryBlockPtr(nullptr),
			mIsValidPtr(std::make_shared<std::atomic<bool>>(false))
		{}
		
		BufferSubAllocationReservation::~BufferSubAllocationReservation()
		{
			ReturnReservation();
		}

		BufferSubAllocationReservation::BufferSubAllocationReservation(BufferSubAllocationReservation&& rhs) noexcept :
			mOwningManagerPtr(rhs.mOwningManagerPtr),
			mMemoryBlockPtr(rhs.mMemoryBlockPtr),
			mIsValidPtr(std::move(rhs.mIsValidPtr))
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

			mIsValidPtr = std::move(rhs.mIsValidPtr);

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

		BufferSubAllocationReservationHandle BufferSubAllocationReservation::CreateHandle()
		{
			assert(mIsValidPtr != nullptr);
			return BufferSubAllocationReservationHandle{ *this, mIsValidPtr };
		}

		void BufferSubAllocationReservation::UpdateValidity()
		{
			assert(mIsValidPtr != nullptr);
			
			const bool isValid = (mOwningManagerPtr != nullptr && mMemoryBlockPtr != nullptr);
			mIsValidPtr->store(isValid, std::memory_order::relaxed);
		}

		void BufferSubAllocationReservation::MarkForDestruction()
		{
			mReadyForDestruction.store(true, std::memory_order::relaxed);
		}

		bool BufferSubAllocationReservation::ReadyForDestruction() const
		{
			return mReadyForDestruction.load(std::memory_order::relaxed);
		}

		void BufferSubAllocationReservation::ReturnReservation()
		{
			if (mOwningManagerPtr != nullptr && mMemoryBlockPtr != nullptr) [[likely]]
			{
				mOwningManagerPtr->DeleteSubAllocation(*this);

				mOwningManagerPtr = nullptr;
				mMemoryBlockPtr = nullptr;

				UpdateValidity();
			}
		}

		void BufferSubAllocationReservation::SetOwningManager(BufferSubAllocationManager& owningManager)
		{
			mOwningManagerPtr = &owningManager;
			UpdateValidity();
		}

		TLSFMemoryBlock& BufferSubAllocationReservation::GetTLSFMemoryBlock() const
		{
			assert(mMemoryBlockPtr != nullptr);
			return *mMemoryBlockPtr;
		}

		void BufferSubAllocationReservation::SetTLSFMemoryBlock(TLSFMemoryBlock& memoryBlock)
		{
			mMemoryBlockPtr = &memoryBlock;
			UpdateValidity();
		}
	}
}