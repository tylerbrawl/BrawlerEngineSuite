module;
#include <vector>
#include <array>
#include <mutex>
#include <optional>

export module Brawler.D3D12.TLSFAllocator;
import Brawler.D3D12.TLSFMemoryBlock;
import Brawler.OptionalRef;
import Brawler.D3D12.TLSFAllocationRequestInfo;

// This is an allocator based off the whitepaper TLSF: A New Dynamic Memory Allocator
// for Real-Time Systems.

namespace Brawler
{
	namespace D3D12
	{
		static constexpr std::size_t SLI = 5; 
		static constexpr std::size_t SECOND_LEVEL_SUBDIVISIONS = (1 << SLI);

		struct PoolSearchInfo
		{
			std::uint32_t FirstLevelIndex;
			std::uint32_t SecondLevelIndex;

			explicit PoolSearchInfo(const std::size_t sizeInBytes);
		};

		class TLSFAllocatorLevelTwoList
		{
		public:
			TLSFAllocatorLevelTwoList() = default;

			TLSFAllocatorLevelTwoList(const TLSFAllocatorLevelTwoList& rhs) = delete;
			TLSFAllocatorLevelTwoList& operator=(const TLSFAllocatorLevelTwoList& rhs) = delete;

			TLSFAllocatorLevelTwoList(TLSFAllocatorLevelTwoList&& rhs) noexcept = default;
			TLSFAllocatorLevelTwoList& operator=(TLSFAllocatorLevelTwoList&& rhs) noexcept = default;

			void InsertBlock(const PoolSearchInfo searchInfo, TLSFMemoryBlock& block);
			TLSFMemoryBlock* TryExtractFreeBlock(const std::uint32_t secondLevelIndex, const TLSFAllocationRequestInfo& allocationInfo);

			void RemoveMergedBlock(const std::uint32_t secondLevelIndex, TLSFMemoryBlock& block);

			bool HasFreeBlocks() const;

		private:
			std::array<TLSFMemoryBlock*, SECOND_LEVEL_SUBDIVISIONS> mFreeBlockListArr;
			std::uint32_t mFreePoolBitMask;
		};

		class TLSFAllocatorLevelOneList
		{
		public:
			TLSFAllocatorLevelOneList() = default;

			TLSFAllocatorLevelOneList(const TLSFAllocatorLevelOneList& rhs) = delete;
			TLSFAllocatorLevelOneList& operator=(const TLSFAllocatorLevelOneList& rhs) = delete;

			TLSFAllocatorLevelOneList(TLSFAllocatorLevelOneList&& rhs) noexcept = default;
			TLSFAllocatorLevelOneList& operator=(TLSFAllocatorLevelOneList&& rhs) noexcept = default;

			void Initialize(const std::size_t heapSizeInBytes);
			
			void InsertBlock(const PoolSearchInfo searchInfo, TLSFMemoryBlock& block);
			TLSFMemoryBlock* TryExtractFreeBlock(const TLSFAllocationRequestInfo& allocationInfo);

			void RemoveMergedBlock(const PoolSearchInfo searchInfo, TLSFMemoryBlock& block);

			bool HasFreeBlocks() const;

		private:
			std::vector<TLSFAllocatorLevelTwoList> mLevelTwoListArr;
			std::uint32_t mFreePoolBitMask;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class TLSFAllocator
		{
		public:
			TLSFAllocator() = default;

			TLSFAllocator(const TLSFAllocator& rhs) = delete;
			TLSFAllocator& operator=(const TLSFAllocator& rhs) = delete;

			TLSFAllocator(TLSFAllocator&& rhs) noexcept = default;
			TLSFAllocator& operator=(TLSFAllocator&& rhs) noexcept = default;

			void Initialize(const std::size_t heapSizeInBytes);

			Brawler::OptionalRef<TLSFMemoryBlock> CreateAllocation(const TLSFAllocationRequestInfo& allocationInfo);
			void DeleteAllocation(TLSFMemoryBlock& memoryBlock);

		private:
			void InsertBlock(TLSFMemoryBlock& block);
			void RemoveMergedBlock(TLSFMemoryBlock& block);

		private:
			std::vector<std::unique_ptr<TLSFMemoryBlock>> mBlockArr;
			TLSFAllocatorLevelOneList mLevelOneList;
			mutable std::mutex mCritSection;
		};
	}
}