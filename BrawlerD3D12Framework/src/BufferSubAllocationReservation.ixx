module;
#include <cstddef>

export module Brawler.D3D12.BufferSubAllocationReservation;

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

		public:
			BufferSubAllocationReservation() = default;
			~BufferSubAllocationReservation();

			BufferSubAllocationReservation(const BufferSubAllocationReservation& rhs) = delete;
			BufferSubAllocationReservation& operator=(const BufferSubAllocationReservation& rhs) = delete;

			BufferSubAllocationReservation(BufferSubAllocationReservation&& rhs) noexcept;
			BufferSubAllocationReservation& operator=(BufferSubAllocationReservation&& rhs) noexcept;

			BufferSubAllocationManager& GetBufferSubAllocationManager() const;

			std::size_t GetReservationSize() const;
			std::size_t GetOffsetFromBufferStart() const;

		private:
			void ReturnReservation();

			void SetOwningManager(BufferSubAllocationManager& owningManager);

			TLSFMemoryBlock& GetTLSFMemoryBlock() const;
			void SetTLSFMemoryBlock(TLSFMemoryBlock& memoryBlock);

		private:
			BufferSubAllocationManager* mOwningManagerPtr;
			TLSFMemoryBlock* mMemoryBlockPtr;
		};
	}
}