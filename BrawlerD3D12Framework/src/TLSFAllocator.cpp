module;
#include <vector>
#include <array>
#include <mutex>
#include <cassert>
#include <optional>
#include <ranges>
#include "DxDef.h"

module Brawler.D3D12.TLSFAllocator;

namespace
{
	template <typename BitmaskType>
	struct BitmaskInfo
	{
		static_assert(sizeof(BitmaskType) != sizeof(BitmaskType));
	};

	template <std::size_t NumRelevantBits>
	struct BitmaskInfoInstantiation
	{
		static constexpr std::size_t NUM_RELEVANT_BITS = NumRelevantBits;
	};

	template <>
	struct BitmaskInfo<std::uint32_t> : public BitmaskInfoInstantiation<32>
	{};

	template <typename BitmaskType>
	std::optional<std::uint32_t> GetNextHighestStorageClassIndex(const std::uint32_t attemptedIndex, const BitmaskType bitMask)
	{
		const std::uint32_t bitmaskToCheck = (bitMask >> attemptedIndex);

		std::uint32_t newIndex = 0;
		const std::uint8_t offsettedIndexResult = _BitScanForward(&(reinterpret_cast<unsigned long&>(newIndex)), bitmaskToCheck);

		if (offsettedIndexResult == 0) [[unlikely]]
			return std::optional<std::uint32_t>{};

		return std::optional<std::uint32_t>{ (attemptedIndex + newIndex) };
	}
}

namespace Brawler
{
	namespace D3D12
	{
		PoolSearchInfo::PoolSearchInfo(const std::size_t sizeInBytes) :
			FirstLevelIndex(),
			SecondLevelIndex()
		{
			// The following equations are from the TLSF whitepaper:
			//
			//   - First-Level Index: f = floor(log_2(sizeInBytes))
			//
			//   - Second-Level Index: s = (sizeInBytes - (2^f)) * ((2^SLI) / (2^f))
			//
			// The calculations that follow are optimized versions of these equations.

			const std::uint8_t firstLevelIndexResult = _BitScanReverse64(&(reinterpret_cast<unsigned long&>(FirstLevelIndex)), sizeInBytes);
			assert(firstLevelIndexResult != 0);

			static constexpr std::size_t SLI_MASK = ((static_cast<std::size_t>(1) << SLI) - 1);
			
			SecondLevelIndex = static_cast<std::uint32_t>((sizeInBytes >> (static_cast<std::size_t>(FirstLevelIndex) - SLI)) & SLI_MASK);
		}

		void TLSFAllocatorLevelTwoList::InsertBlock(const PoolSearchInfo searchInfo, TLSFMemoryBlock& block)
		{
			assert(searchInfo.SecondLevelIndex < mFreeBlockListArr.size());
			
			if (mFreeBlockListArr[searchInfo.SecondLevelIndex] != nullptr)
			{
				mFreeBlockListArr[searchInfo.SecondLevelIndex]->SetPreviousFreeBlock(&block);
				block.SetNextFreeBlock(mFreeBlockListArr[searchInfo.SecondLevelIndex]);
			}

			mFreeBlockListArr[searchInfo.SecondLevelIndex] = &block;

			// Update the bitmask to account for the fact that the list at searchInfo.SecondLevelIndex
			// now has at least one free block.
			mFreePoolBitMask |= (static_cast<std::uint32_t>(1) << searchInfo.SecondLevelIndex);
		}

		TLSFMemoryBlock* TLSFAllocatorLevelTwoList::TryExtractFreeBlock(const std::uint32_t secondLevelIndex, const TLSFAllocationRequestInfo& allocationInfo)
		{
			const auto extractFreeBlockLambda = [this] (const std::uint32_t secondLevelIndex, const TLSFAllocationRequestInfo& allocationInfo) -> TLSFMemoryBlock*
			{
				TLSFMemoryBlock* extractedBlockPtr = mFreeBlockListArr[secondLevelIndex];

				if (extractedBlockPtr != nullptr)
				{
					if (extractedBlockPtr->CanBlockAllocateResource(allocationInfo))
					{
						mFreeBlockListArr[secondLevelIndex] = extractedBlockPtr->GetNextFreeBlock();

						if (mFreeBlockListArr[secondLevelIndex] != nullptr)
							mFreeBlockListArr[secondLevelIndex]->SetPreviousFreeBlock(nullptr);
						else
							mFreePoolBitMask &= ~(static_cast<std::uint32_t>(1) << secondLevelIndex);
					}
					else
						extractedBlockPtr = nullptr;
				}

				return extractedBlockPtr;
			};

			// First, we try to get a block from the same storage class as is specified by
			// the required allocation size.
			TLSFMemoryBlock* freeBlockPtr = extractFreeBlockLambda(secondLevelIndex, allocationInfo);

			if (freeBlockPtr != nullptr)
				return freeBlockPtr;

			// If that failed, then we get a block from the next largest storage class. This
			// can be done in constant time by using intrinsics.
			const std::optional<std::uint32_t> newSecondLevelIndex = GetNextHighestStorageClassIndex(secondLevelIndex, mFreePoolBitMask);

			if (!newSecondLevelIndex.has_value()) [[unlikely]]
				return nullptr;

			// We have obtained the index of the next-largest storage class. If we can't allocate from this,
			// then we just give up.

			return extractFreeBlockLambda(*newSecondLevelIndex, allocationInfo);
		}

		void TLSFAllocatorLevelTwoList::RemoveMergedBlock(const std::uint32_t secondLevelIndex, TLSFMemoryBlock& block)
		{
			if (mFreeBlockListArr[secondLevelIndex] == &block)
			{
				mFreeBlockListArr[secondLevelIndex] = block.GetNextFreeBlock();

				if (mFreeBlockListArr[secondLevelIndex] != nullptr)
					mFreeBlockListArr[secondLevelIndex]->SetPreviousFreeBlock(nullptr);
				else
					mFreePoolBitMask &= ~(static_cast<std::uint32_t>(1) << secondLevelIndex);
			}
		}

		bool TLSFAllocatorLevelTwoList::HasFreeBlocks() const
		{
			return (mFreePoolBitMask != 0);
		}

		void TLSFAllocatorLevelOneList::Initialize(const std::size_t heapSizeInBytes)
		{
			// From the TLSF whitepaper: FLI = min(log_2(heapSizeInBytes), 32).
			//
			// The paper's equation says that 31 is actually the limit, but other parts of
			// the paper imply that 32 is the real value. The 31 seems like errata to me.

			std::uint32_t fli = 0;
			const std::uint8_t fliResult = _BitScanReverse64(&(reinterpret_cast<unsigned long&>(fli)), heapSizeInBytes);
			assert(fliResult != 0);

			mLevelTwoListArr.resize(std::min<std::uint32_t>(fli + 1, 32));
		}

		void TLSFAllocatorLevelOneList::InsertBlock(const PoolSearchInfo searchInfo, TLSFMemoryBlock& block)
		{
			assert(searchInfo.FirstLevelIndex < mLevelTwoListArr.size());

			mLevelTwoListArr[searchInfo.FirstLevelIndex].InsertBlock(searchInfo, block);

			// Update the bitmask to account for the fact that the list at 
			// mLevelTwoListArr[searchInfo.FirstLevelIndex] now has at least one pool with a free block.
			mFreePoolBitMask |= (static_cast<std::uint32_t>(1) << searchInfo.FirstLevelIndex);
		}

		TLSFMemoryBlock* TLSFAllocatorLevelOneList::TryExtractFreeBlock(const TLSFAllocationRequestInfo& allocationInfo)
		{
			const auto extractFromLevelTwoListLambda = [this] (const PoolSearchInfo searchInfo, const TLSFAllocationRequestInfo& allocationInfo) -> TLSFMemoryBlock*
			{
				TLSFMemoryBlock* freeBlockPtr = mLevelTwoListArr[searchInfo.FirstLevelIndex].TryExtractFreeBlock(searchInfo.SecondLevelIndex, allocationInfo);

				// If necessary, update the bitmask to account for the list which we just extracted a block
				// from now being empty.
				if (freeBlockPtr != nullptr && !mLevelTwoListArr[searchInfo.FirstLevelIndex].HasFreeBlocks())
					mFreePoolBitMask &= ~(static_cast<std::uint32_t>(1) << searchInfo.FirstLevelIndex);

				return freeBlockPtr;
			};

			// First, we try to get a block from the same storage class as is specified by
			// the required allocation size.
			PoolSearchInfo searchInfo{ allocationInfo.SizeInBytes };
			TLSFMemoryBlock* freeBlockPtr = extractFromLevelTwoListLambda(searchInfo, allocationInfo);

			if (freeBlockPtr != nullptr)
				return freeBlockPtr;

			// If that failed, then we get a block from the next largest storage class. 
			// 
			// In the original TLSF whitepaper, This was done in constant time by using intrinsics.
			// However, the paper never accounted for alignment, whereas our TLSFAllocator *must*
			// guarantee correctness for alignment. Thus, we need to loop if we do not want to waste
			// memory.
			//
			// The idea of a constant-time search is nice, but some of the data types in the D3D12
			// API have ridiculous alignment requirements. For instance, UAV counters must be placed
			// at an offset D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT (4,096 bytes) from the start of
			// the buffer. That can result in a lot of wasted space!
			while (true)
			{
				const std::optional<std::uint32_t> newFirstLevelIndex = GetNextHighestStorageClassIndex(searchInfo.FirstLevelIndex, mFreePoolBitMask);

				if (!newFirstLevelIndex.has_value()) [[unlikely]]
					return nullptr;

				// We have obtained the index of the next-largest storage class. If we can't allocate from this,
				// then move on to the next storage class. In the original paper, the algorithm would have ended
				// there with an allocation failure. However, the original paper never took alignment into account.

				searchInfo.FirstLevelIndex = *newFirstLevelIndex;
				searchInfo.SecondLevelIndex = 0;

				TLSFMemoryBlock* const higherClassFreeBlockPtr = extractFromLevelTwoListLambda(searchInfo, allocationInfo);

				if (higherClassFreeBlockPtr != nullptr)
					return higherClassFreeBlockPtr;

				// Increment the index to prevent an infinite loop.
				++(searchInfo.FirstLevelIndex);
			}
		}

		void TLSFAllocatorLevelOneList::RemoveMergedBlock(const PoolSearchInfo searchInfo, TLSFMemoryBlock& block)
		{
			mLevelTwoListArr[searchInfo.FirstLevelIndex].RemoveMergedBlock(searchInfo.SecondLevelIndex, block);

			// If necessary, update the bitmask to account for the list which we just extracted a block
			// from now being empty.
			if (!mLevelTwoListArr[searchInfo.FirstLevelIndex].HasFreeBlocks())
				mFreePoolBitMask &= ~(static_cast<std::uint32_t>(1) << searchInfo.FirstLevelIndex);
		}

		bool TLSFAllocatorLevelOneList::HasFreeBlocks() const
		{
			return (mFreePoolBitMask != 0);
		}

		void TLSFAllocator::Initialize(const std::size_t heapSizeInBytes)
		{
			mLevelOneList.Initialize(heapSizeInBytes);

			// The first block in the heap represents the entire allocated memory range.
			std::unique_ptr<TLSFMemoryBlock> initialBlock{ std::make_unique<TLSFMemoryBlock>() };
			initialBlock->SetBlockSize(heapSizeInBytes);

			InsertBlock(*initialBlock);
			mBlockArr.push_back(std::move(initialBlock));
		}

		Brawler::OptionalRef<TLSFMemoryBlock> TLSFAllocator::CreateAllocation(const TLSFAllocationRequestInfo& allocationInfo)
		{
			TLSFMemoryBlock* freeBlockPtr = nullptr;
			
			{
				std::scoped_lock<std::mutex> lock{ mCritSection };

				if (!mLevelOneList.HasFreeBlocks())
					return Brawler::OptionalRef<TLSFMemoryBlock>{};

				// Try to extract an unused block from the pool. If we get back nullptr, then
				// this heap is incapable of allocating the resource.
				freeBlockPtr = mLevelOneList.TryExtractFreeBlock(allocationInfo);

				if (freeBlockPtr == nullptr)
					return Brawler::OptionalRef<TLSFMemoryBlock>{};

				// We were able to find a block which can hold this resource. Now, we want to use it as an
				// allocation. In doing so, we might create up to two additional blocks: possibly one to
				// the immediate left of freeBlockPtr (for the sake of alignment) and possibly one to its
				// immediate right (in the event of any free space left in the block).
				std::vector<std::unique_ptr<TLSFMemoryBlock>> splitBlockArr{ freeBlockPtr->AllocateBlock(allocationInfo) };

				for (auto& createdBlock : splitBlockArr)
				{
					InsertBlock(*createdBlock);
					mBlockArr.push_back(std::move(createdBlock));
				}
			}

			return Brawler::OptionalRef<TLSFMemoryBlock>{ *freeBlockPtr };
		}

		void TLSFAllocator::DeleteAllocation(TLSFMemoryBlock& memoryBlock)
		{
			{
				std::scoped_lock<std::mutex> lock{ mCritSection };

				assert(std::ranges::find_if(mBlockArr, [blockPtr = &memoryBlock] (const std::unique_ptr<TLSFMemoryBlock>& block) { return blockPtr == block.get(); }) != mBlockArr.end() && "ERROR: An attempt was made to return a TLSFMemoryBlock to a TLSFAllocator which did not own it!");

				// When we proceed to free the block, we are given a list of up to two blocks which
				// we can delete from mBlockArr. This is because these blocks were merged into a
				// different block. Specifically, the two blocks which may be included in the returned
				// std::vector are the block itself (having been merged into its left neighbor) and
				// the block's right neighbor (having been merged into either the original block or its
				// left neighbor).
				std::vector<TLSFMemoryBlock*> deleteableBlockArr{ memoryBlock.FreeBlock() };

				const bool wasAllocationBlockMerged = (std::ranges::find(deleteableBlockArr, &memoryBlock) != deleteableBlockArr.end());

				for (const auto deleteableBlockPtr : deleteableBlockArr)
				{
					RemoveMergedBlock(*deleteableBlockPtr);
					std::erase_if(mBlockArr, [deleteableBlockPtr] (const std::unique_ptr<TLSFMemoryBlock>& block) { return (deleteableBlockPtr == block.get()); });
				}

				// If the original allocation was never merged into an adjacent block, then we add it
				// to the list.
				if (!wasAllocationBlockMerged)
					InsertBlock(memoryBlock);
			}
		}

		void TLSFAllocator::InsertBlock(TLSFMemoryBlock& block)
		{
			mLevelOneList.InsertBlock(PoolSearchInfo{ block.GetBlockSize() }, block);
		}

		void TLSFAllocator::RemoveMergedBlock(TLSFMemoryBlock& block)
		{
			mLevelOneList.RemoveMergedBlock(PoolSearchInfo{ block.GetBlockSize() }, block);
		}
	}
}