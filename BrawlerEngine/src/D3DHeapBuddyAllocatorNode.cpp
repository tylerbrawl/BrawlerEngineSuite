module;
#include <cassert>
#include "DxDef.h"

module Brawler.IMPL.D3DHeapBuddyAllocatorNode;
import Util.Math;
import Brawler.D3DHeapBuddyAllocator;

namespace Brawler
{
	namespace IMPL
	{
		D3DHeapBuddyAllocatorNode* D3DHeapBuddyAllocatorNode::FindSuitableNodeForReservation(const D3D12_RESOURCE_ALLOCATION_INFO& allocInfo)
		{
			if (Reserved)
				return nullptr;

			// If it is possible, then try to defer responsibility to the children nodes.
			const std::uint64_t childSize = (SizeInBytes / 2);
			if (childSize >= allocInfo.SizeInBytes && childSize >= Allocator->GetMinimumNodeSize())
			{
				CreateChildNodes();

				D3DHeapBuddyAllocatorNode* childResult = GetLeftChild().FindSuitableNodeForReservation(allocInfo);
				if (childResult != nullptr)
					return childResult;

				return GetRightChild().FindSuitableNodeForReservation(allocInfo);
			}

			// At this point, we find that none of the child nodes are capable of making this
			// reservation, but the size of this node is large enough. However, it *MIGHT* not 
			// be an exact fit.
			//
			// So, what we need to do is first align the offset to the required alignment.
			// If the allocation then still fits entirely within the node, then we use it
			// for the allocation.
			const std::uint64_t alignmentDifference = Util::Math::AlignUp(Offset, allocInfo.Alignment) - Offset;
			return ((alignmentDifference + allocInfo.SizeInBytes <= AvailableSizeInBytes) ? this : nullptr);
		}

		void D3DHeapBuddyAllocatorNode::CreateReservation()
		{
			// Even if the allocation does not use all of the bytes designated to a node,
			// we will still mark all of it as being reserved.
			AvailableSizeInBytes = 0;
			Reserved = true;

			if (ParentNode != nullptr)
				ParentNode->UpdateAvailableSize();
		}

		void D3DHeapBuddyAllocatorNode::DeleteReservation()
		{
			AvailableSizeInBytes = SizeInBytes;
			Reserved = false;

			if (ParentNode != nullptr)
				ParentNode->UpdateAvailableSize();
		}

		void D3DHeapBuddyAllocatorNode::CreateChildNodes()
		{
			if (LeftChildNode == nullptr)
				LeftChildNode = std::make_unique<D3DHeapBuddyAllocatorNode>(*this, Offset);

			if (RightChildNode == nullptr)
				RightChildNode = std::make_unique<D3DHeapBuddyAllocatorNode>(*this, Offset + (SizeInBytes / 2));
		}

		D3DHeapBuddyAllocatorNode& D3DHeapBuddyAllocatorNode::GetLeftChild()
		{
			assert(LeftChildNode != nullptr);
			return *(LeftChildNode.get());
		}

		D3DHeapBuddyAllocatorNode& D3DHeapBuddyAllocatorNode::GetRightChild()
		{
			assert(RightChildNode != nullptr);
			return *(RightChildNode.get());
		}

		void D3DHeapBuddyAllocatorNode::UpdateAvailableSize()
		{
			assert(LeftChildNode != nullptr && RightChildNode != nullptr);
			AvailableSizeInBytes = std::max(LeftChildNode->AvailableSizeInBytes, RightChildNode->AvailableSizeInBytes);

			if (ParentNode != nullptr)
				ParentNode->UpdateAvailableSize();
		}
	}
}