module;
#include <cstdint>
#include <cassert>

export module Brawler.D3DHeapAllocationHandle;
import Brawler.IMPL.D3DHeapBuddyAllocatorNode;

export namespace Brawler
{
	class D3DHeap;
	class D3DHeapBuddyAllocator;
	class I_GPUResource;
}

export namespace Brawler
{
	// NOTE: Despite the name having the word "handle" in it, this structure actually
	// contains several pointer-sized variables. If you need to pass it around, then
	// try to do so by reference, when applicable.
	struct D3DHeapAllocationHandle
	{
	private:
		friend class D3DHeap;
		friend class D3DHeapBuddyAllocator;
		friend class I_GPUResource;
		friend struct IMPL::D3DHeapBuddyAllocatorNode;

	private:
		D3DHeap* OwningHeap;
		IMPL::D3DHeapBuddyAllocatorNode* AllocatedNode;
		std::uint64_t Offset;
		std::uint64_t Size;

	public:
		D3DHeapAllocationHandle() :
			OwningHeap(nullptr),
			AllocatedNode(nullptr),
			Offset(0),
			Size(0)
		{}

		D3DHeapAllocationHandle(
			IMPL::D3DHeapBuddyAllocatorNode& allocatedNode,
			const std::uint64_t offset,
			const std::uint64_t size
		) :
			OwningHeap(nullptr),
			AllocatedNode(&allocatedNode),
			Offset(offset),
			Size(size)
		{
			AllocatedNode->CreateReservation();
		}

		~D3DHeapAllocationHandle()
		{
			if (AllocatedNode != nullptr)
				AllocatedNode->DeleteReservation();
		}

		D3DHeapAllocationHandle(const D3DHeapAllocationHandle& rhs) = delete;
		D3DHeapAllocationHandle& operator=(const D3DHeapAllocationHandle& rhs) = delete;

		D3DHeapAllocationHandle(D3DHeapAllocationHandle&& rhs) noexcept :
			OwningHeap(rhs.OwningHeap),
			AllocatedNode(rhs.AllocatedNode),
			Offset(rhs.Offset),
			Size(rhs.Size)
		{
			rhs = D3DHeapAllocationHandle{};
		}

		D3DHeapAllocationHandle& operator=(D3DHeapAllocationHandle&& rhs) noexcept
		{
			OwningHeap = rhs.OwningHeap;
			rhs.OwningHeap = nullptr;

			AllocatedNode = rhs.AllocatedNode;
			rhs.AllocatedNode = nullptr;

			Offset = rhs.Offset;
			rhs.Offset = 0;

			Size = rhs.Size;
			rhs.Size = 0;

			return *this;
		}
	};
}