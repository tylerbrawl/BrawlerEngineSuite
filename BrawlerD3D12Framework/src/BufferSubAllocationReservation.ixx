module;
#include <memory>
#include <atomic>

export module Brawler.D3D12.BufferSubAllocationReservation;
import Brawler.D3D12.BufferSubAllocationReservationHandle;

export namespace Brawler
{
	namespace D3D12
	{
		class BufferSubAllocationManager;
		class TLSFMemoryBlock;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class BufferSubAllocationReservation
		{
		private:
			friend class BufferSubAllocationManager;
			friend class BufferSubAllocationReservationHandle;

		public:
			BufferSubAllocationReservation();
			~BufferSubAllocationReservation();

			BufferSubAllocationReservation(const BufferSubAllocationReservation& rhs) = delete;
			BufferSubAllocationReservation& operator=(const BufferSubAllocationReservation& rhs) = delete;

			BufferSubAllocationReservation(BufferSubAllocationReservation&& rhs) noexcept;
			BufferSubAllocationReservation& operator=(BufferSubAllocationReservation&& rhs) noexcept;

			BufferSubAllocationManager& GetBufferSubAllocationManager() const;

			std::size_t GetReservationSize() const;
			std::size_t GetOffsetFromBufferStart() const;

			BufferSubAllocationReservationHandle CreateHandle();

		private:
			void UpdateValidity();

			void MarkForDestruction();
			bool ReadyForDestruction() const;

			void ReturnReservation();

			void SetOwningManager(BufferSubAllocationManager& owningManager);

			TLSFMemoryBlock& GetTLSFMemoryBlock() const;
			void SetTLSFMemoryBlock(TLSFMemoryBlock& memoryBlock);

		private:
			BufferSubAllocationManager* mOwningManagerPtr;
			TLSFMemoryBlock* mMemoryBlockPtr;
			std::atomic<bool> mReadyForDestruction;
			std::shared_ptr<std::atomic<bool>> mIsValidPtr;
		};
	}
}