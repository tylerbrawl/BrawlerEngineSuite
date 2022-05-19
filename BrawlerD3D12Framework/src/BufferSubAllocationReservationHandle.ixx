module;
#include <memory>
#include <atomic>

export module Brawler.D3D12.BufferSubAllocationReservationHandle;

export namespace Brawler
{
	namespace D3D12
	{
		class BufferSubAllocationReservation;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class BufferSubAllocationReservationHandle
		{
		public:
			BufferSubAllocationReservationHandle() = default;
			BufferSubAllocationReservationHandle(BufferSubAllocationReservation& reservation, std::shared_ptr<std::atomic<bool>> validityChecker);

			~BufferSubAllocationReservationHandle();

			BufferSubAllocationReservationHandle(const BufferSubAllocationReservationHandle& rhs) = delete;
			BufferSubAllocationReservationHandle& operator=(const BufferSubAllocationReservationHandle& rhs) = delete;

			BufferSubAllocationReservationHandle(BufferSubAllocationReservationHandle&& rhs) noexcept;
			BufferSubAllocationReservationHandle& operator=(BufferSubAllocationReservationHandle&& rhs) noexcept;

			bool IsReservationValid() const;

			BufferSubAllocationReservation& operator*();
			const BufferSubAllocationReservation& operator*() const;

			BufferSubAllocationReservation* operator->();
			const BufferSubAllocationReservation* operator->() const;

		private:
			void MarkReservationForDestruction();

		private:
			BufferSubAllocationReservation* mReservationPtr;
			std::shared_ptr<std::atomic<bool>> mValidityCheckerPtr;
		};
	}
}