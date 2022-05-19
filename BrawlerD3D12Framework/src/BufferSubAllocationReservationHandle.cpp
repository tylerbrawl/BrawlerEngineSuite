module;
#include <memory>
#include <atomic>
#include <cassert>

module Brawler.D3D12.BufferSubAllocationReservationHandle;
import Brawler.D3D12.BufferSubAllocationReservation;

namespace Brawler
{
	namespace D3D12
	{
		BufferSubAllocationReservationHandle::BufferSubAllocationReservationHandle(BufferSubAllocationReservation& reservation, std::shared_ptr<std::atomic<bool>> validityChecker) :
			mReservationPtr(&reservation),
			mValidityCheckerPtr(std::move(validityChecker))
		{}

		BufferSubAllocationReservationHandle::~BufferSubAllocationReservationHandle()
		{
			MarkReservationForDestruction();
		}

		BufferSubAllocationReservationHandle::BufferSubAllocationReservationHandle(BufferSubAllocationReservationHandle&& rhs) noexcept :
			mReservationPtr(rhs.mReservationPtr),
			mValidityCheckerPtr(std::move(rhs.mValidityCheckerPtr))
		{
			rhs.mReservationPtr = nullptr;
		}

		BufferSubAllocationReservationHandle& BufferSubAllocationReservationHandle::operator=(BufferSubAllocationReservationHandle&& rhs) noexcept
		{
			MarkReservationForDestruction();
			
			mReservationPtr = rhs.mReservationPtr;
			rhs.mReservationPtr = nullptr;

			mValidityCheckerPtr = std::move(rhs.mValidityCheckerPtr);

			return *this;
		}

		bool BufferSubAllocationReservationHandle::IsReservationValid() const
		{
			// The reservation is valid iff mValidityCheckerPtr->load() returns true (assuming,
			// of course, that mReservationPtr != nullptr).
			
			return (mReservationPtr != nullptr && mValidityCheckerPtr != nullptr && mValidityCheckerPtr->load(std::memory_order::relaxed));
		}

		BufferSubAllocationReservation& BufferSubAllocationReservationHandle::operator*()
		{
			assert(IsReservationValid());
			return *mReservationPtr;
		}

		const BufferSubAllocationReservation& BufferSubAllocationReservationHandle::operator*() const
		{
			assert(IsReservationValid());
			return *mReservationPtr;
		}

		BufferSubAllocationReservation* BufferSubAllocationReservationHandle::operator->()
		{
			assert(IsReservationValid());
			return mReservationPtr;
		}

		const BufferSubAllocationReservation* BufferSubAllocationReservationHandle::operator->() const
		{
			assert(IsReservationValid());
			return mReservationPtr;
		}

		void BufferSubAllocationReservationHandle::MarkReservationForDestruction()
		{
			if (IsReservationValid())
			{
				mReservationPtr->MarkForDestruction();

				mReservationPtr = nullptr;
				mValidityCheckerPtr = nullptr;
			}
		}
	}
}