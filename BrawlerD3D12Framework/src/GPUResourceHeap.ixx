module;
#include <cassert>
#include <mutex>
#include <algorithm>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceHeap;
import Util.Engine;
import Brawler.D3D12.GPUResourceLifetimeType;
import Brawler.D3D12.TLSFAllocator;
import Brawler.D3D12.GPUResourceAllocation;
import Brawler.D3D12.I_PageableGPUObject;
import Brawler.OptionalRef;
import Brawler.D3D12.TLSFAllocationRequestInfo;

// More MSVC modules jank...
export import Brawler.D3D12.GPUResourceAllocationInfo;
export import Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		class GPUResourceHeap final : public I_PageableGPUObject
		{
		public:
			GPUResourceHeap() = default;
			
			GPUResourceHeap(const GPUResourceHeap& rhs) = delete;
			GPUResourceHeap& operator=(const GPUResourceHeap& rhs) = delete;

			GPUResourceHeap(GPUResourceHeap&& rhs) noexcept = default;
			GPUResourceHeap& operator=(GPUResourceHeap&& rhs) noexcept = default;

			HRESULT InitializeHeap(const D3D12_HEAP_FLAGS heapFlags, const std::size_t sizeInBytes);

			bool IsHeapAlive() const;

			/// <summary>
			/// Attempts to create a GPUResourceAllocation within this GPUResourceHeap instance for
			/// a set of I_GPUResource instances which are to be aliased in memory.
			/// 
			/// NOTE: Since the provided I_GPUResource instances are going to be aliased, allocating
			/// memory for resources which are to be placed in two different memory locations means
			/// making multiple calls to this function.
			/// </summary>
			/// <param name="allocationInfo">
			/// - A GPUResourceAllocationInfo instance which contains the resources to be aliased,
			///   the required allocation size, and the required allocation alignment.
			/// </param>
			/// <returns>
			/// The function returns one of the following HRESULT values based on the result of
			/// the operation:
			/// 
			///  - S_OK: The resource(s) were successfully allocated a place in memory within
			///    this GPUResourceHeap instance.
			/// 
			///  - E_OUTOFMEMORY: The heap does not have enough free space to allocate the resources.
			///    This could be due to fragmentation, alignment requirements, or simply just a lack
			///    of free space.
			/// 
			///  - E_ILLEGAL_METHOD_CALL: The function was called while the heap was evicted.
			///    One can call GPUResourceHeap::IsResident() (inherited from I_PageableGPUObject)
			///    to determine if a given heap is resident in GPU memory or not.
			/// </returns>
			HRESULT AllocateResources(const GPUResourceAllocationInfo& allocationInfo);

			/// <summary>
			/// Returns the size, in bytes, of the underlying ID3D12Heap.
			/// </summary>
			/// <returns>
			/// The function returns the size, in bytes, of the underlying ID3D12Heap.
			/// </returns>
			std::size_t GetSize() const;

		private:
			bool NeedsResidency() const override;
			float GetUsageMetric() const override;
			std::size_t GetGPUMemorySize() const override;
			ID3D12Pageable& GetD3D12PageableObject() const override;
			bool IsDeletionSafe() const override;
			void DeleteD3D12PageableObject() override;

			void DeleteUnusedAllocations();

		private:
			Microsoft::WRL::ComPtr<Brawler::D3D12Heap> mHeap;
			TLSFAllocator mAllocator;
			std::vector<std::unique_ptr<GPUResourceAllocation>> mOwnedAllocationArr;
			std::size_t mSizeInBytes;
			mutable std::mutex mCritSection;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		HRESULT GPUResourceHeap<HeapType, LifetimeType>::InitializeHeap(const D3D12_HEAP_FLAGS heapFlags, const std::size_t sizeInBytes)
		{
			static const CD3DX12_HEAP_PROPERTIES heapProperties{ HeapType };

			std::scoped_lock<std::mutex> lock{ mCritSection };
			
			// Ensure that the specified size is a power of two.
			assert(__popcnt64(sizeInBytes) == 1 && "ERROR: An attempt was made to create an ID3D12Heap with a non-power-of-two size in bytes!");

			// Ensure that the specified size is at least as big as the heap's alignment.
			assert(sizeInBytes >= D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT && "ERROR: An attempt was made to create an ID3D12Heap with a size less than the required heap alignment!");

			const D3D12_HEAP_DESC heapDesc{
				.SizeInBytes = sizeInBytes,
				.Properties = heapProperties,
				.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,

				// As an optimization, ensure that the OS is allowed to elide zeroing the heap
				// before we use it.
				.Flags = (heapFlags | D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_CREATE_NOT_ZEROED)
			};

			Microsoft::WRL::ComPtr<ID3D12Heap> createdHeap{};
			HRESULT hr = Util::Engine::GetD3D12Device().CreateHeap(&heapDesc, IID_PPV_ARGS(&createdHeap));

			if (FAILED(hr)) [[unlikely]]
				return hr;

			hr = createdHeap.As(&mHeap);

			if (FAILED(hr)) [[unlikely]]
				return hr;

			mSizeInBytes = sizeInBytes;
			mAllocator.Initialize(sizeInBytes);

			// Set the residency status based on the provided D3D12_HEAP_FLAGS.
			SetResidencyStatus((heapFlags & D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) == 0);

			return S_OK;
		}

		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		bool GPUResourceHeap<HeapType, LifetimeType>::IsHeapAlive() const
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			return (mHeap != nullptr);
		}

		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		HRESULT GPUResourceHeap<HeapType, LifetimeType>::AllocateResources(const GPUResourceAllocationInfo& allocationInfo)
		{
			// If the heap is currently evicted, then we need to make it resident before we can
			// allocate resources within it.
			if (!IsResident()) [[unlikely]]
				return E_ILLEGAL_METHOD_CALL;
			
			{
				std::scoped_lock<std::mutex> lock{ mCritSection };

				assert(mHeap != nullptr);

				// Check for any allocations which we can delete. GPUResourceAllocations keep track of
				// which I_GPUResources were assigned to them, and are notified of when they are destroyed.
				// When an allocation no longer has any resources, we can (and should) delete it.
				DeleteUnusedAllocations();

				const TLSFAllocationRequestInfo requestInfo{
					.SizeInBytes = allocationInfo.AllocationInfo.SizeInBytes,
					.Alignment = allocationInfo.AllocationInfo.Alignment
				};
				Brawler::OptionalRef<TLSFMemoryBlock> memoryBlock{ mAllocator.CreateAllocation(requestInfo) };

				if (!memoryBlock.HasValue())
					return E_OUTOFMEMORY;

				std::unique_ptr<GPUResourceAllocation> allocation{ std::make_unique<GPUResourceAllocation>() };
				allocation->SetD3D12Heap(*(mHeap.Get()));
				allocation->SetTLSFBlock(*memoryBlock);

				// Initialize each resource. One might argue that we could move this outside of the lock
				// and do it after pushing allocation into mOwnedAllocationArr. However, we need to do this 
				// while locked so that DeleteUnusedAllocations(), if called on another thread, does not try
				// to delete the allocation we created before we can assign it a resource.
				for (auto& resourcePtr : allocationInfo.AliasedResources)
					allocation->AssignGPUResource(*resourcePtr);

				// Push back the newly created GPUResourceAllocation instance. This GPUResourceHeap now
				// owns this allocation. However, the I_GPUResource instances still manage their own
				// ID3D12Resources.
				//
				// What is the point of a GPUResourceAllocation, then? Not only does it retain the information
				// regarding which resources are aliased to each other, but it also provides us a means to
				// track its usage without modifying the I_GPUResource instances themselves. This can be
				// used as heuristics for determining when/if an I_GPUResource should be moved around.
				mOwnedAllocationArr.push_back(std::move(allocation));
			}
			
			return S_OK;
		}

		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		std::size_t GPUResourceHeap<HeapType, LifetimeType>::GetSize() const
		{
			// This value never changes after the heap is initialized, so we technically don't
			// need a mutex here.

			return mSizeInBytes;
		}

		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		bool GPUResourceHeap<HeapType, LifetimeType>::NeedsResidency() const
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			for (const auto& allocationPtr : mOwnedAllocationArr)
			{
				if (!(allocationPtr->CouldAllocatedResourcesBeEvicted()))
					return true;
			}

			return false;
		}

		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		float GPUResourceHeap<HeapType, LifetimeType>::GetUsageMetric() const
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };
			
			float maximumUsageMetric = 0.0f;

			for (const auto& allocationPtr : mOwnedAllocationArr)
				maximumUsageMetric = std::max<float>(maximumUsageMetric, allocationPtr->GetMaximumGPUResourceUsageMetric());

			return maximumUsageMetric;
		}

		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		std::size_t GPUResourceHeap<HeapType, LifetimeType>::GetGPUMemorySize() const
		{
			return GetSize();
		}

		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		ID3D12Pageable& GPUResourceHeap<HeapType, LifetimeType>::GetD3D12PageableObject() const
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			assert(mHeap != nullptr);
			return static_cast<ID3D12Pageable&>(*(mHeap.Get()));
		}

		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		bool GPUResourceHeap<HeapType, LifetimeType>::IsDeletionSafe() const
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			// The GPUResourceHeap instance can be deleted iff it is no longer housing any
			// active resources.
			for (const auto& allocationPtr : mOwnedAllocationArr)
			{
				if (allocationPtr->HasGPUResourceAssignments())
					return false;
			}

			return true;
		}

		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		void GPUResourceHeap<HeapType, LifetimeType>::DeleteD3D12PageableObject()
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			mHeap.Reset();
		}

		template <D3D12_HEAP_TYPE HeapType, GPUResourceLifetimeType LifetimeType>
		void GPUResourceHeap<HeapType, LifetimeType>::DeleteUnusedAllocations()
		{
			// *LOCKED*
			//
			// This is called from within a locked context.

			for (auto itr = mOwnedAllocationArr.begin(); itr != mOwnedAllocationArr.end();)
			{
				if (!((*itr)->HasGPUResourceAssignments()))
				{
					mAllocator.DeleteAllocation((*itr)->GetTLSFBlock());
					itr = mOwnedAllocationArr.erase(itr);
				}
				else
					++itr;
			}
		}
	}
}