module;
#include <memory>
#include <optional>
#include <mutex>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceHeapAllocator;
import Brawler.D3D12.GPUResourceLifetimeType;

namespace Brawler
{
	namespace D3D12
	{
		struct GPUResourceHeapAllocatorNode
		{
			/// <summary>
			/// This is the offset, in bytes, from the start of the heap to the beginning
			/// of the memory range represented by this node.
			/// </summary>
			std::size_t HeapOffsetInBytes;

			/// <summary>
			/// This is the size, in bytes, of the memory range represented by this node.
			/// </summary>
			std::size_t NodeSizeInBytes;

			bool CurrentlyInUse;

			GPUResourceHeapAllocatorNode* ParentNode;
			std::unique_ptr<GPUResourceHeapAllocatorNode> LeftChild;
			std::unique_ptr<GPUResourceHeapAllocatorNode> RightChild;

			void CreateChildNodes();
			GPUResourceHeapAllocatorNode* FindAvailableNode(const D3D12_RESOURCE_ALLOCATION_INFO& allocationInfo);

			void ClaimNode();
			void ReleaseNode();
		};

		class GPUResourceHeapAllocator;

		class GPUResourceHeapAllocationIMPL
		{
		public:
			GPUResourceHeapAllocationIMPL(GPUResourceHeapAllocator& allocator, GPUResourceHeapAllocatorNode& node, Brawler::D3D12Heap& d3d12Heap);
			~GPUResourceHeapAllocationIMPL();

			GPUResourceHeapAllocationIMPL(const GPUResourceHeapAllocationIMPL& rhs) = delete;
			GPUResourceHeapAllocationIMPL& operator=(const GPUResourceHeapAllocationIMPL& rhs) = delete;

			GPUResourceHeapAllocationIMPL(GPUResourceHeapAllocationIMPL&& rhs) noexcept;
			GPUResourceHeapAllocationIMPL& operator=(GPUResourceHeapAllocationIMPL&& rhs) noexcept;

			Brawler::D3D12Heap& GetD3D12Heap();
			const Brawler::D3D12Heap& GetD3D12Heap() const;

			std::size_t GetHeapOffset() const;

		private:
			GPUResourceHeapAllocator* mAllocatorPtr;
			GPUResourceHeapAllocatorNode* mNodePtr;
			Brawler::D3D12Heap* mD3D12HeapPtr;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceHeapAllocation
		{
		private:
			friend class GPUResourceHeapAllocator;

		public:
			GPUResourceHeapAllocation() = default;

		private:
			explicit GPUResourceHeapAllocation(GPUResourceHeapAllocationIMPL&& allocation);

		public:
			~GPUResourceHeapAllocation() = default;

			GPUResourceHeapAllocation(const GPUResourceHeapAllocation& rhs) = default;
			GPUResourceHeapAllocation& operator=(const GPUResourceHeapAllocation& rhs) = default;

			GPUResourceHeapAllocation(GPUResourceHeapAllocation&& rhs) noexcept = default;
			GPUResourceHeapAllocation& operator=(GPUResourceHeapAllocation&& rhs) noexcept = default;

			Brawler::D3D12Heap& GetD3D12Heap();
			const Brawler::D3D12Heap& GetD3D12Heap() const;

			std::size_t GetHeapOffset() const;

		private:
			std::shared_ptr<GPUResourceHeapAllocationIMPL> mSharedAllocation;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceHeapAllocator
		{
		private:
			friend class GPUResourceHeapAllocationIMPL;

		public:
			GPUResourceHeapAllocator() = default;

			GPUResourceHeapAllocator(const GPUResourceHeapAllocator& rhs) = delete;
			GPUResourceHeapAllocator& operator=(const GPUResourceHeapAllocator& rhs) = delete;

			GPUResourceHeapAllocator(GPUResourceHeapAllocator&& rhs) noexcept = default;
			GPUResourceHeapAllocator& operator=(GPUResourceHeapAllocator&& rhs) noexcept = default;

			void Initialize(const GPUResourceLifetimeType lifetimeType, Brawler::D3D12Heap& owningD3D12Heap, const std::size_t heapSizeInBytes);

			std::optional<GPUResourceHeapAllocation> CreateAllocation(const D3D12_RESOURCE_ALLOCATION_INFO& allocationInfo);

			/// <summary>
			/// Describes whether or not this GPUResourceHeapAllocator instance has any current allocations.
			/// </summary>
			/// <returns>
			/// The function returns true if this GPUResourceHeapAllocator instance has any current allocations
			/// and false otherwise.
			/// </returns>
			bool HasAllocations() const;

			GPUResourceLifetimeType GetGPUResourceLifetimeType() const;

		private:
			void DeleteAllocation(GPUResourceHeapAllocatorNode& node);

		private:
			std::unique_ptr<GPUResourceHeapAllocatorNode> mHeadNode;
			std::size_t mHeapSize;
			Brawler::D3D12Heap* mOwningD3D12Heap;
			GPUResourceLifetimeType mLifetimeType;
			mutable std::mutex mCritSection;
		};
	}
}