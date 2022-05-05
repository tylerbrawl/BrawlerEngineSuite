module;
#include <cstdint>
#include <cassert>
#include <memory>
#include <optional>
#include "DxDef.h"

module Brawler.D3DHeapBuddyAllocator;
import Brawler.D3DHeap;
import Brawler.ResourceCreationInfo;
import Brawler.D3DHeapAllocationHandle;
import Util.Math;
import Brawler.IMPL.D3DHeapBuddyAllocatorNode;

namespace Brawler
{
	D3DHeapBuddyAllocator::D3DHeapBuddyAllocator() :
		mHeadNode(nullptr),
		mMinimumNodeSize(0)
	{}

	void D3DHeapBuddyAllocator::Initialize(const Brawler::D3DHeap& owningHeap)
	{
		// Set the minimum node size to be that of the small alignment of the owningHeap, if
		// it is allowed.
		if (owningHeap.GetHeapInfo().SmallAlignment)
			mMinimumNodeSize = *(owningHeap.GetHeapInfo().SmallAlignment);
		else
			mMinimumNodeSize = owningHeap.GetHeapInfo().DefaultAlignment;

		// Create the root node of the tree.
		mHeadNode = std::make_unique<IMPL::D3DHeapBuddyAllocatorNode>(*this, owningHeap.GetHeapSize());
	}

	std::optional<D3DHeapAllocationHandle> D3DHeapBuddyAllocator::Allocate(const D3D12_RESOURCE_ALLOCATION_INFO& allocInfo)
	{
		// Make sure that we are doing a properly-aligned allocation.
		assert(Util::Math::IsAligned(allocInfo.SizeInBytes, GetMinimumNodeSize()) && "ERROR: An attempt was made to perform an unaligned allocation within a D3DHeap!");

		if (GetLargestAvailableRegionSize() < allocInfo.SizeInBytes)
			return std::optional<D3DHeapAllocationHandle>{};

		IMPL::D3DHeapBuddyAllocatorNode* suitableNode = mHeadNode->FindSuitableNodeForReservation(allocInfo);
		if (suitableNode == nullptr)
			return std::optional<D3DHeapAllocationHandle>{};

		D3DHeapAllocationHandle hAllocation{ *suitableNode, Util::Math::AlignUp(suitableNode->Offset, allocInfo.Alignment), allocInfo.SizeInBytes };
		return std::optional<D3DHeapAllocationHandle>{ std::move(hAllocation) };
	}

	std::uint64_t D3DHeapBuddyAllocator::GetLargestAvailableRegionSize() const
	{
		assert(mHeadNode != nullptr && "ERROR: An attempt was made to use a D3DHeapBuddyAllocator before it was initialized!");

		return mHeadNode->AvailableSizeInBytes;
	}

	std::uint64_t D3DHeapBuddyAllocator::GetMinimumNodeSize() const
	{
		return mMinimumNodeSize;
	}
}