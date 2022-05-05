module;
#include <memory>
#include <optional>
#include <mutex>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceHeapAllocator;

namespace
{
	static constexpr std::size_t MINIMUM_ALLOCATOR_NODE_SIZE = D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;
}

namespace Brawler
{
	namespace D3D12
	{
		void GPUResourceHeapAllocatorNode::CreateChildNodes()
		{
			LeftChild = std::make_unique<GPUResourceHeapAllocatorNode>(GPUResourceHeapAllocatorNode{
				.HeapOffsetInBytes = HeapOffsetInBytes,
				.NodeSizeInBytes = (NodeSizeInBytes / 2),
				.CurrentlyInUse = false,
				.ParentNode = this,
				.LeftChild{ nullptr },
				.RightChild{ nullptr }
			});

			RightChild = std::make_unique<GPUResourceHeapAllocatorNode>(GPUResourceHeapAllocatorNode{
				.HeapOffsetInBytes = (HeapOffsetInBytes + (NodeSizeInBytes / 2)),
				.NodeSizeInBytes = (NodeSizeInBytes / 2),
				.CurrentlyInUse = false,
				.ParentNode = this,
				.LeftChild{ nullptr },
				.RightChild{ nullptr }
			});
		}

		GPUResourceHeapAllocatorNode* GPUResourceHeapAllocatorNode::FindAvailableNode(const D3D12_RESOURCE_ALLOCATION_INFO& allocationInfo)
		{
			if (NodeSizeInBytes < allocationInfo.SizeInBytes)
				return nullptr;

			const std::size_t childNodeSize = (NodeSizeInBytes / 2);

			// If a child node can still fit the requested data, then we prefer to send it to those.
			if (childNodeSize >= allocationInfo.SizeInBytes)
			{
				if (LeftChild == nullptr || RightChild == nullptr)
					CreateChildNodes();

				if (!LeftChild->CurrentlyInUse)
					return LeftChild->FindAvailableNode(allocationInfo);

				if (!RightChild->CurrentlyInUse)
					return RightChild->FindAvailableNode(allocationInfo);

				return nullptr;
			}

			// Otherwise, this node is suitable iff it is not currently in use (which implies that its
			// children also are not being used) *AND* the offset from the start of the heap is a
			// multiple of the required alignment.
			return (!CurrentlyInUse && (HeapOffsetInBytes % allocationInfo.Alignment == 0) ? this : nullptr);
		}

		void GPUResourceHeapAllocatorNode::ClaimNode()
		{
			GPUResourceHeapAllocatorNode* currNodePtr = this;

			while (currNodePtr != nullptr)
			{
				currNodePtr->CurrentlyInUse = true;
				currNodePtr = currNodePtr->ParentNode;
			}
		}

		void GPUResourceHeapAllocatorNode::ReleaseNode()
		{
			CurrentlyInUse = false;
			GPUResourceHeapAllocatorNode* currNodePtr = ParentNode;

			while (currNodePtr != nullptr)
			{
				assert(currNodePtr->LeftChild != nullptr && currNodePtr->RightChild != nullptr);

				// If neither the left child nor the right child is being used, then the
				// parent node is also not being used.
				if (!currNodePtr->LeftChild->CurrentlyInUse && !currNodePtr->RightChild->CurrentlyInUse)
				{
					CurrentlyInUse = false;
					currNodePtr = currNodePtr->ParentNode;
				}
				else
					break;
			}
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		GPUResourceHeapAllocationIMPL::GPUResourceHeapAllocationIMPL(GPUResourceHeapAllocator& allocator, GPUResourceHeapAllocatorNode& node, Brawler::D3D12Heap& d3d12Heap) :
			mAllocatorPtr(&allocator),
			mNodePtr(&node),
			mD3D12HeapPtr(&d3d12Heap)
		{}

		GPUResourceHeapAllocationIMPL::~GPUResourceHeapAllocationIMPL()
		{
			if (mAllocatorPtr != nullptr && mNodePtr != nullptr)
				mAllocatorPtr->DeleteAllocation(*mNodePtr);
		}

		GPUResourceHeapAllocationIMPL::GPUResourceHeapAllocationIMPL(GPUResourceHeapAllocationIMPL&& rhs) noexcept :
			mAllocatorPtr(rhs.mAllocatorPtr),
			mNodePtr(rhs.mNodePtr),
			mD3D12HeapPtr(rhs.mD3D12HeapPtr)
		{
			rhs.mAllocatorPtr = nullptr;
			rhs.mNodePtr = nullptr;
			rhs.mD3D12HeapPtr = nullptr;
		}

		GPUResourceHeapAllocationIMPL& GPUResourceHeapAllocationIMPL::operator=(GPUResourceHeapAllocationIMPL&& rhs) noexcept
		{
			mAllocatorPtr = rhs.mAllocatorPtr;
			rhs.mAllocatorPtr = nullptr;

			mNodePtr = rhs.mNodePtr;
			rhs.mNodePtr = nullptr;

			mD3D12HeapPtr = rhs.mD3D12HeapPtr;
			rhs.mD3D12HeapPtr = nullptr;

			return *this;
		}

		Brawler::D3D12Heap& GPUResourceHeapAllocationIMPL::GetD3D12Heap()
		{
			return *mD3D12HeapPtr;
		}

		const Brawler::D3D12Heap& GPUResourceHeapAllocationIMPL::GetD3D12Heap() const
		{
			return *mD3D12HeapPtr;
		}

		std::size_t GPUResourceHeapAllocationIMPL::GetHeapOffset() const
		{
			return (mNodePtr->HeapOffsetInBytes);
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		GPUResourceHeapAllocation::GPUResourceHeapAllocation(GPUResourceHeapAllocationIMPL&& allocation) :
			mSharedAllocation(std::make_shared<GPUResourceHeapAllocationIMPL>(std::move(allocation)))
		{}

		Brawler::D3D12Heap& GPUResourceHeapAllocation::GetD3D12Heap()
		{
			assert(mSharedAllocation != nullptr);
			return mSharedAllocation->GetD3D12Heap();
		}

		const Brawler::D3D12Heap& GPUResourceHeapAllocation::GetD3D12Heap() const
		{
			assert(mSharedAllocation != nullptr);
			return mSharedAllocation->GetD3D12Heap();
		}

		std::size_t GPUResourceHeapAllocation::GetHeapOffset() const
		{
			assert(mSharedAllocation != nullptr);
			return mSharedAllocation->GetHeapOffset();
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		void GPUResourceHeapAllocator::Initialize(const GPUResourceLifetimeType lifetimeType, Brawler::D3D12Heap& owningD3D12Heap, const std::size_t heapSizeInBytes)
		{
			mLifetimeType = lifetimeType;
			mHeapSize = heapSizeInBytes;
			mOwningD3D12Heap = &owningD3D12Heap;

			mHeadNode = std::make_unique<GPUResourceHeapAllocatorNode>(GPUResourceHeapAllocatorNode{
				.HeapOffsetInBytes = 0,
				.NodeSizeInBytes = heapSizeInBytes,
				.CurrentlyInUse = false,
				.ParentNode = nullptr,
				.LeftChild{ nullptr },
				.RightChild{ nullptr }
			});
		}

		std::optional<GPUResourceHeapAllocation> GPUResourceHeapAllocator::CreateAllocation(const D3D12_RESOURCE_ALLOCATION_INFO& allocationInfo)
		{
			assert(allocationInfo.SizeInBytes >= MINIMUM_ALLOCATOR_NODE_SIZE && "ERROR: An attempt was made to make an allocation into a GPUResourceHeap with a size smaller than the alignment of a small D3D12 resource (4KB)!");

			std::optional<GPUResourceHeapAllocation> allocation{};

			// Exit early if the requested size is too big.
			if (allocationInfo.SizeInBytes > mHeapSize) [[unlikely]]
				return allocation;

			{
				std::scoped_lock<std::mutex> lock{ mCritSection };

				GPUResourceHeapAllocatorNode* availableNode = mHeadNode->FindAvailableNode(allocationInfo);

				if (availableNode != nullptr)
				{
					availableNode->ClaimNode();
					allocation = GPUResourceHeapAllocation{ GPUResourceHeapAllocationIMPL{ *this, *availableNode, *mOwningD3D12Heap } };
				}
			}
			
			return allocation;
		}

		bool GPUResourceHeapAllocator::HasAllocations() const
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			assert(mHeadNode != nullptr);
			return mHeadNode->CurrentlyInUse;
		}

		GPUResourceLifetimeType GPUResourceHeapAllocator::GetGPUResourceLifetimeType() const
		{
			return mLifetimeType;
		}

		void GPUResourceHeapAllocator::DeleteAllocation(GPUResourceHeapAllocatorNode& node)
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			node.ReleaseNode();
		}
	}
}