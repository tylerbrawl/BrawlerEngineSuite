module;
#include <vector>
#include <memory>
#include <cassert>

export module Brawler.D3D12.TLSFMemoryBlock;
import Brawler.D3D12.TLSFAllocationRequestInfo;

export namespace Brawler
{
	namespace D3D12
	{
		class TLSFMemoryBlock
		{
		public:
			TLSFMemoryBlock();

			bool CanBlockAllocateResource(const TLSFAllocationRequestInfo& allocationInfo) const;
			std::vector<std::unique_ptr<TLSFMemoryBlock>> AllocateBlock(const TLSFAllocationRequestInfo& allocationInfo);

			std::vector<TLSFMemoryBlock*> FreeBlock();

			__forceinline void SetBlockSize(const std::size_t sizeInBytes);
			__forceinline std::size_t GetBlockSize() const;

			__forceinline void SetHeapOffset(const std::size_t heapOffset);
			__forceinline std::size_t GetHeapOffset() const;

			__forceinline void SetPreviousFreeBlock(TLSFMemoryBlock* blockPtr);
			__forceinline TLSFMemoryBlock* GetPreviousFreeBlock() const;

			__forceinline void SetNextFreeBlock(TLSFMemoryBlock* blockPtr);
			__forceinline TLSFMemoryBlock* GetNextFreeBlock() const;

		private:
			__forceinline void SetFreeStatus(const bool isBlockFree);
			__forceinline bool IsBlockFree() const;

			__forceinline std::size_t CalculateAlignmentAdjustment(const TLSFAllocationRequestInfo& allocationInfo) const;

		private:
			std::size_t mSizeAndFreeStatus;
			std::size_t mHeapOffset;
			TLSFMemoryBlock* mPrevPhysicalBlockPtr;
			TLSFMemoryBlock* mNextPhysicalBlockPtr;
			TLSFMemoryBlock* mPrevFreeBlockPtr;
			TLSFMemoryBlock* mNextFreeBlockPtr;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		__forceinline void TLSFMemoryBlock::SetBlockSize(const std::size_t sizeInBytes)
		{
			assert((std::numeric_limits<decltype(mSizeAndFreeStatus)>::max() >> 1) >= sizeInBytes && "ERROR: An allocation size so large was provided to a TLSFMemoryBlock that it couldn't fit in 63 bits!");

			mSizeAndFreeStatus = ((sizeInBytes << 1) | (mSizeAndFreeStatus & 0x1));
		}

		__forceinline std::size_t TLSFMemoryBlock::GetBlockSize() const
		{
			return (mSizeAndFreeStatus >> 1);
		}

		__forceinline void TLSFMemoryBlock::SetHeapOffset(const std::size_t heapOffset)
		{
			mHeapOffset = heapOffset;
		}

		__forceinline std::size_t TLSFMemoryBlock::GetHeapOffset() const
		{
			return mHeapOffset;
		}

		__forceinline void TLSFMemoryBlock::SetPreviousFreeBlock(TLSFMemoryBlock* blockPtr)
		{
			mPrevFreeBlockPtr = blockPtr;
		}

		__forceinline TLSFMemoryBlock* TLSFMemoryBlock::GetPreviousFreeBlock() const
		{
			return mPrevFreeBlockPtr;
		}

		__forceinline void TLSFMemoryBlock::SetNextFreeBlock(TLSFMemoryBlock* blockPtr)
		{
			mNextFreeBlockPtr = blockPtr;
		}

		__forceinline TLSFMemoryBlock* TLSFMemoryBlock::GetNextFreeBlock() const
		{
			return mNextFreeBlockPtr;
		}
	}
}