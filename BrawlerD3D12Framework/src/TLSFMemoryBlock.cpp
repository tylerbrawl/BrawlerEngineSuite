module;
#include <vector>
#include <memory>
#include <cassert>

module Brawler.D3D12.TLSFMemoryBlock;
import Util.Math;

namespace Brawler
{
	namespace D3D12
	{
		TLSFMemoryBlock::TLSFMemoryBlock() :
			mSizeAndFreeStatus(1),  // Start the block in the free state.
			mHeapOffset(0),
			mPrevPhysicalBlockPtr(nullptr),
			mNextPhysicalBlockPtr(nullptr),
			mNextFreeBlockPtr(nullptr)
		{}
		
		__forceinline void TLSFMemoryBlock::SetFreeStatus(const bool isBlockFree)
		{
			mSizeAndFreeStatus = ((mSizeAndFreeStatus & (std::numeric_limits<std::size_t>::max() - 1)) | (isBlockFree ? 1 : 0));
		}

		__forceinline bool TLSFMemoryBlock::IsBlockFree() const
		{
			return ((mSizeAndFreeStatus & 0x1) != 0);
		}

		__forceinline std::size_t TLSFMemoryBlock::CalculateAlignmentAdjustment(const TLSFAllocationRequestInfo& allocationInfo) const
		{
			assert(Util::Math::IsPowerOfTwo(allocationInfo.Alignment));
			
			return (Util::Math::AlignToPowerOfTwo(GetHeapOffset(), allocationInfo.Alignment) - GetHeapOffset());
		}

		bool TLSFMemoryBlock::CanBlockAllocateResource(const TLSFAllocationRequestInfo& allocationInfo) const
		{
			if (!IsBlockFree())
				return false;

			const std::size_t alignmentAdjustment = CalculateAlignmentAdjustment(allocationInfo);

			if (alignmentAdjustment > GetBlockSize())
				return false;

			return ((GetBlockSize() - alignmentAdjustment) >= allocationInfo.SizeInBytes);
		}

		std::vector<std::unique_ptr<TLSFMemoryBlock>> TLSFMemoryBlock::AllocateBlock(const TLSFAllocationRequestInfo& allocationInfo)
		{
			assert(CanBlockAllocateResource(allocationInfo) && "ERROR: An attempt was made to have a TLSFMemoryBlock allocate a resource which it did not have enough space for!");

			std::vector<std::unique_ptr<TLSFMemoryBlock>> splitBlockArr{};
			const std::size_t alignmentAdjustment = CalculateAlignmentAdjustment(allocationInfo);

			// If we need to adjust for alignment, then we have to create a free block to the left
			// of this one.
			if (alignmentAdjustment != 0)
			{
				std::unique_ptr<TLSFMemoryBlock> alignmentPaddingBlock{ std::make_unique<TLSFMemoryBlock>() };

				alignmentPaddingBlock->SetBlockSize(alignmentAdjustment);
				SetBlockSize(GetBlockSize() - alignmentAdjustment);

				alignmentPaddingBlock->SetHeapOffset(GetHeapOffset());
				SetHeapOffset(GetHeapOffset() + alignmentAdjustment);

				alignmentPaddingBlock->mPrevPhysicalBlockPtr = mPrevPhysicalBlockPtr;

				if (mPrevPhysicalBlockPtr != nullptr)
					mPrevPhysicalBlockPtr->mNextPhysicalBlockPtr = alignmentPaddingBlock.get();

				mPrevPhysicalBlockPtr = alignmentPaddingBlock.get();

				alignmentPaddingBlock->mNextPhysicalBlockPtr = this;

				splitBlockArr.push_back(std::move(alignmentPaddingBlock));
			}

			assert(CanBlockAllocateResource(allocationInfo));

			// At this point, we know that the heap offset of this block is correctly aligned to
			// the requirements specified by allocationInfo. Now, we want to only allocate as
			// much memory as is required. We can create a new block for any remaining space.
			const std::size_t freeMemorySize = (GetBlockSize() - allocationInfo.SizeInBytes);
			if (freeMemorySize > 0) [[likely]]
			{
				std::unique_ptr<TLSFMemoryBlock> freeMemoryBlock{ std::make_unique<TLSFMemoryBlock>() };

				freeMemoryBlock->SetBlockSize(freeMemorySize);
				SetBlockSize(allocationInfo.SizeInBytes);

				freeMemoryBlock->SetHeapOffset(GetHeapOffset() + allocationInfo.SizeInBytes);

				freeMemoryBlock->mPrevPhysicalBlockPtr = this;

				freeMemoryBlock->mNextPhysicalBlockPtr = mNextPhysicalBlockPtr;

				if (mNextPhysicalBlockPtr != nullptr)
					mNextPhysicalBlockPtr->mPrevPhysicalBlockPtr = freeMemoryBlock.get();

				mNextPhysicalBlockPtr = freeMemoryBlock.get();

				splitBlockArr.push_back(std::move(freeMemoryBlock));
			}

			// Mark the current block as no longer free.
			SetFreeStatus(false);

			return splitBlockArr;
		}

		std::vector<TLSFMemoryBlock*> TLSFMemoryBlock::FreeBlock()
		{
			assert(!IsBlockFree() && "ERROR: An attempt was made to free a TLSFMemoryBlock before it was used!");

			std::vector<TLSFMemoryBlock*> deleteableBlockArr{};

			// First, try to merge the right adjacent block into this one.
			if (mNextPhysicalBlockPtr != nullptr && mNextPhysicalBlockPtr->IsBlockFree())
			{
				SetBlockSize(GetBlockSize() + mNextPhysicalBlockPtr->GetBlockSize());

				TLSFMemoryBlock* const newNextPhysicalBlockPtr = mNextPhysicalBlockPtr->mNextPhysicalBlockPtr;

				if (newNextPhysicalBlockPtr != nullptr)
					newNextPhysicalBlockPtr->mPrevPhysicalBlockPtr = this;

				if (mNextPhysicalBlockPtr->mPrevFreeBlockPtr != nullptr)
					mNextPhysicalBlockPtr->mPrevFreeBlockPtr->mNextFreeBlockPtr = mNextPhysicalBlockPtr->mNextFreeBlockPtr;
				if (mNextPhysicalBlockPtr->mNextFreeBlockPtr != nullptr)
					mNextPhysicalBlockPtr->mNextFreeBlockPtr->mPrevFreeBlockPtr = mNextPhysicalBlockPtr->mPrevFreeBlockPtr;

				deleteableBlockArr.push_back(mNextPhysicalBlockPtr);
				mNextPhysicalBlockPtr = newNextPhysicalBlockPtr;
			}

			// After that, try to merge this block into the right adjacent one. By doing it in this
			// order, we can potentially merge all three blocks easily into one block.
			if (mPrevPhysicalBlockPtr != nullptr && mPrevPhysicalBlockPtr->IsBlockFree())
			{
				mPrevPhysicalBlockPtr->SetBlockSize(mPrevPhysicalBlockPtr->GetBlockSize() + GetBlockSize());

				if (mNextPhysicalBlockPtr != nullptr)
					mNextPhysicalBlockPtr->mPrevPhysicalBlockPtr = mPrevPhysicalBlockPtr;

				if (mPrevFreeBlockPtr != nullptr)
					mPrevFreeBlockPtr->mNextFreeBlockPtr = mNextFreeBlockPtr;
				if (mNextFreeBlockPtr != nullptr)
					mNextFreeBlockPtr->mPrevFreeBlockPtr = mPrevFreeBlockPtr;

				deleteableBlockArr.push_back(this);
				mPrevPhysicalBlockPtr->mNextPhysicalBlockPtr = mNextPhysicalBlockPtr;
			}

			// Mark the current block as free again.
			SetFreeStatus(true);

			return deleteableBlockArr;
		}
	}
}