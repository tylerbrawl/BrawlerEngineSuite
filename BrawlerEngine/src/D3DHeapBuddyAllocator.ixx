module;
#include <memory>
#include <optional>
#include "DxDef.h"

export module Brawler.D3DHeapBuddyAllocator;
import Brawler.D3DHeapAllocationHandle;
import Util.Math;
import Brawler.IMPL.D3DHeapBuddyAllocatorNode;

export namespace Brawler
{
	class D3DHeap;
}

export namespace Brawler
{
	class D3DHeapBuddyAllocator
	{
	private:
		friend struct IMPL::D3DHeapBuddyAllocatorNode;

	public:
		D3DHeapBuddyAllocator();

	public:
		D3DHeapBuddyAllocator(const D3DHeapBuddyAllocator& rhs) = delete;
		D3DHeapBuddyAllocator& operator=(const D3DHeapBuddyAllocator& rhs) = delete;

		D3DHeapBuddyAllocator(D3DHeapBuddyAllocator&& rhs) noexcept = default;
		D3DHeapBuddyAllocator& operator=(D3DHeapBuddyAllocator&& rhs) noexcept = default;

		void Initialize(const Brawler::D3DHeap& owningHeap);

		/// <summary>
		/// Attempts to reserve sizeInBytes bytes from the owning D3DHeap. The allocation will
		/// be in a contiguous region in memory, and will be given the proper alignment.
		/// </summary>
		/// <param name="sizeInBytes">
		/// - The size, in bytes, of the allocation to be made.
		/// </param>
		/// <returns>
		/// If successful, then the returned std::optional instance has a valid handle which
		/// represents the allocation made for this call. Otherwise, the returned std::optional
		/// instance has no value.
		/// </returns>
		std::optional<Brawler::D3DHeapAllocationHandle> Allocate(const D3D12_RESOURCE_ALLOCATION_INFO& allocInfo);

		/// <summary>
		/// This function returns a conservative estimate of the size, in bytes, of the largest 
		/// available contiguous memory region in the tree. 
		/// 
		/// Due to fragmentation and the tree-like nature of the algorithm, this may not actually 
		/// be the largest available size, it will never return a value lower than the actual 
		/// available size.
		/// 
		/// This is not necessarily a bad thing, however, since the tree structure guarantees that
		/// if an allocation can be made, then it will be made at an aligned memory address.
		/// </summary>
		/// <returns>
		/// A conservative estimate (see the description) of the size, in bytes, of the largest 
		/// available contiguous memory region in the tree.
		/// </returns>
		std::uint64_t GetLargestAvailableRegionSize() const;

	private:
		std::uint64_t GetMinimumNodeSize() const;

	private:
		std::unique_ptr<IMPL::D3DHeapBuddyAllocatorNode> mHeadNode;
		std::uint64_t mMinimumNodeSize;
	};
}