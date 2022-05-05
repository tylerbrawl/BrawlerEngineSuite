module;
#include <cstdint>
#include <memory>
#include "DxDef.h"

export module Brawler.IMPL.D3DHeapBuddyAllocatorNode;

export namespace Brawler
{
	class D3DHeapBuddyAllocator;
}

export namespace Brawler
{
	namespace IMPL
	{
		struct D3DHeapBuddyAllocatorNode
		{
			// This is the size, in bytes, represented by this node.
			std::uint64_t SizeInBytes;

			// This is the offset, in bytes, from the beginning of the heap of this node.
			std::uint64_t Offset;

			// This is the largest contiguous range of memory available
			// amongst all child nodes. 
			// 
			// As memory becomes deallocated, there may be a contiguous segment 
			// of memory which cannot be allocated due to fragmentation and the 
			// inherent tree structure of the buddy allocation system. This field 
			// does *NOT* merge these segments together to return a combined size 
			// of "available" contiguous memory, since reserving it would break the 
			// tree structure.
			std::uint64_t AvailableSizeInBytes;

			bool Reserved;
			std::unique_ptr<D3DHeapBuddyAllocatorNode> LeftChildNode;
			std::unique_ptr<D3DHeapBuddyAllocatorNode> RightChildNode;
			D3DHeapBuddyAllocatorNode* ParentNode;
			const D3DHeapBuddyAllocator* const Allocator;

			// Creates a D3DHeapBuddyAllocatorNode as a root node.
			D3DHeapBuddyAllocatorNode(const D3DHeapBuddyAllocator& buddyAllocator, const std::uint64_t sizeInBytes) :
				Offset(0),
				SizeInBytes(sizeInBytes),
				AvailableSizeInBytes(sizeInBytes),
				Reserved(false),
				LeftChildNode(nullptr),
				RightChildNode(nullptr),
				ParentNode(nullptr),
				Allocator(&buddyAllocator)
			{}

			// Creates a D3DHeapBuddyAllocatorNode as a child node.
			D3DHeapBuddyAllocatorNode(D3DHeapBuddyAllocatorNode& parentNode, const std::uint64_t offsetInBytes) :
				Offset(offsetInBytes),
				SizeInBytes((parentNode.SizeInBytes) / 2),
				AvailableSizeInBytes(SizeInBytes),
				Reserved(false),
				LeftChildNode(nullptr),
				RightChildNode(nullptr),
				ParentNode(&parentNode),
				Allocator(parentNode.Allocator)
			{}

			D3DHeapBuddyAllocatorNode* FindSuitableNodeForReservation(const D3D12_RESOURCE_ALLOCATION_INFO& allocInfo);
			void CreateReservation();
			void DeleteReservation();

		private:
			void CreateChildNodes();
			D3DHeapBuddyAllocatorNode& GetLeftChild();
			D3DHeapBuddyAllocatorNode& GetRightChild();
			void UpdateAvailableSize();
		};
	}
}