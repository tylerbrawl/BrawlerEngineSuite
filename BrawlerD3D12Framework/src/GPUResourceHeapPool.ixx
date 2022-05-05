module;
#include <vector>
#include <mutex>
#include <optional>
#include <memory>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceHeapPool;
import Brawler.D3D12.GPUResourceHeap;
import Util.Math;
import Brawler.D3D12.GPUResourceAllocationInfo;
import Brawler.D3D12.GPUResourceLifetimeType;

export namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		class GPUResourceHeapPool
		{
		public:
			GPUResourceHeapPool() = default;

			GPUResourceHeapPool(const GPUResourceHeapPool& rhs) = delete;
			GPUResourceHeapPool& operator=(const GPUResourceHeapPool& rhs) = delete;

			GPUResourceHeapPool(GPUResourceHeapPool&& rhs) noexcept = default;
			GPUResourceHeapPool& operator=(GPUResourceHeapPool&& rhs) noexcept = default;

			void Initialize(const D3D12_HEAP_FLAGS heapFlags);

			HRESULT AllocateResources(const GPUResourceAllocationInfo& allocationInfo);

		private:
			void CheckForDeletedHeaps();

		private:
			std::vector<std::unique_ptr<GPUResourceHeap<HeapType, LifetimeType>>> mHeaps;
			D3D12_HEAP_FLAGS mHeapFlags;
			mutable std::mutex mCritSection;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		static constexpr std::size_t MINIMUM_HEAP_SIZE = Util::Math::MegabytesToBytes(256);
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		void GPUResourceHeapPool<HeapType, LifetimeType>::Initialize(const D3D12_HEAP_FLAGS heapFlags)
		{
			// Always try to first create heaps which are resident.
			mHeapFlags = (heapFlags & ~D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT);
		}

		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		HRESULT GPUResourceHeapPool<HeapType, LifetimeType>::AllocateResources(const GPUResourceAllocationInfo& allocationInfo)
		{
			// Ensure that the initial resource state is consistent with the requirements for the
			// defined heap type. We don't need to lock the critical section for this.
#ifdef _DEBUG
			if constexpr (HeapType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
			{
				for (const auto& resourcePtr : allocationInfo.AliasedResources)
					assert(resourcePtr->GetCurrentResourceState() == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ && "ERROR: GPU resources created in an upload heap *MUST* have an initial resource state of D3D12_RESOURCE_STATE_GENERIC_READ!");
			}

			else if constexpr (HeapType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK)
			{
				for(const auto& resourcePtr : allocationInfo.AliasedResources)
					assert(resourcePtr->GetCurrentResourceState() == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST && "ERROR: GPU resources created in a readback heap *MUST* have an initial resource state of D3D12_RESOURCE_STATE_COPY_DEST!");
			}
#endif // _DEBUG

			std::scoped_lock<std::mutex> lock{ mCritSection };

			// First, check for any heaps which the GPUResidencyManager deleted.
			CheckForDeletedHeaps();
			
			// Next, try to allocate the resources within an existing heap.
			for (auto& heap : mHeaps)
			{
				const HRESULT hr = heap->AllocateResources(allocationInfo);

				if (SUCCEEDED(hr))
					return hr;
			}

			// If that failed, then we have no choice but to try to create a new
			// heap.
			std::unique_ptr<GPUResourceHeap<HeapType, LifetimeType>> createdHeapPtr{ std::make_unique<GPUResourceHeap<HeapType, LifetimeType>>() };

			const std::size_t heapSize = std::max<std::size_t>(MINIMUM_HEAP_SIZE, Util::Math::RaiseToPowerOfTwo(allocationInfo.AllocationInfo.SizeInBytes));
			HRESULT hr = createdHeapPtr->InitializeHeap(mHeapFlags, heapSize);

			// If that failed, then we try to make the heap non-resident.
			if (FAILED(hr)) [[unlikely]]
			{
				const D3D12_HEAP_FLAGS evictedHeapFlags = (mHeapFlags | D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT);
				hr = createdHeapPtr->InitializeHeap(evictedHeapFlags, heapSize);

				// If THAT failed, then we appear to have run out of memory.
				if (FAILED(hr)) [[unlikely]]
					return E_OUTOFMEMORY;
			}

			// Otherwise, we create the resource within the heap and move it into the array
			// of resident heaps.
			GPUResourceHeap<HeapType, LifetimeType>& createdHeap{ *(createdHeapPtr.get()) };
			mHeaps.push_back(std::move(createdHeapPtr));

			return createdHeap.AllocateResources(allocationInfo);
		}

		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		void GPUResourceHeapPool<HeapType, LifetimeType>::CheckForDeletedHeaps()
		{
			// *LOCKED*
			//
			// This is called from within a locked context.

			for (auto itr = mHeaps.begin(); itr != mHeaps.end();)
			{
				if (!(*itr)->IsHeapAlive())
					itr = mHeaps.erase(itr);
				else
					++itr;
			}
		}
	}
}