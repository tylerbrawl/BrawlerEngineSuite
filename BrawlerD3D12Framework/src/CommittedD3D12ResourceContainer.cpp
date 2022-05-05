module;
#include <optional>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.CommittedD3D12ResourceContainer;
import Brawler.D3D12.GPUResourceInitializationInfo;
import Util.Engine;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceUsageTracker;

namespace Brawler
{
	namespace D3D12
	{
		CommittedD3D12ResourceContainer::CommittedD3D12ResourceContainer(I_GPUResource& owningResource) :
			D3D12ResourceContainer(owningResource),
			I_PageableGPUObject()
		{}

		void CommittedD3D12ResourceContainer::CreateCommittedD3D12Resource(const D3D12_HEAP_FLAGS heapFlags, const GPUResourceInitializationInfo& initInfo)
		{
			const D3D12_CLEAR_VALUE* optimizedClearValuePtr = nullptr;
			const std::optional<D3D12_CLEAR_VALUE> optimizedClearValue{ GetGPUResource().GetOptimizedClearValue() };

			// Buffers cannot specify an optimized clear value.
			if (initInfo.ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER && optimizedClearValue.has_value()) [[unlikely]]
				optimizedClearValuePtr = &(*optimizedClearValue);

			// Create the properties associated with the heap.
			const D3D12_HEAP_PROPERTIES heapProperties{
				.Type = initInfo.HeapType,
				.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
				.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN,
				.CreationNodeMask = 0,
				.VisibleNodeMask = 0
			};

			// Set the residency status based on the provided D3D12_HEAP_FLAGS.
			SetResidencyStatus((heapFlags & D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT) == 0);

			Microsoft::WRL::ComPtr<Brawler::D3D12Resource> d3dResource{};
			CheckHRESULT(Util::Engine::GetD3D12Device().CreateCommittedResource2(
				&heapProperties,

				// Try to always elide zeroing the heap as an optimization.
				(heapFlags | D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_CREATE_NOT_ZEROED),

				&(initInfo.ResourceDesc),
				initInfo.InitialResourceState,
				optimizedClearValuePtr,
				nullptr,
				IID_PPV_ARGS(&d3dResource)
			));

			SetD3D12Resource(std::move(d3dResource));
		}

		bool CommittedD3D12ResourceContainer::NeedsResidency() const
		{
			return !(GetGPUResource().GetUsageTracker().IsResourceSafeToEvict());
		}

		float CommittedD3D12ResourceContainer::GetUsageMetric() const
		{
			return GetGPUResource().GetUsageTracker().GetResourceUsageMetric();
		}

		std::size_t CommittedD3D12ResourceContainer::GetGPUMemorySize() const
		{
			const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ GetGPUResource().GetResourceDescription() };
			D3D12_RESOURCE_ALLOCATION_INFO1 allocInfo1{};

			Util::Engine::GetD3D12Device().GetResourceAllocationInfo2(
				0,
				1,
				&resourceDesc,
				&allocInfo1
			);

			return allocInfo1.SizeInBytes;
		}

		ID3D12Pageable& CommittedD3D12ResourceContainer::GetD3D12PageableObject() const
		{
			return static_cast<ID3D12Pageable&>(GetGPUResource().GetD3D12Resource());
		}

		bool CommittedD3D12ResourceContainer::IsDeletionSafe() const
		{
			// It is never safe to delete a resource. If it is no longer needed, then we expect the
			// actual I_GPUResource instance to also be deleted. We might be able to evict the
			// resource if it is never being used, but deleting it while the I_GPUResource is
			// alive is a no-go.

			return false;
		}

		void CommittedD3D12ResourceContainer::DeleteD3D12PageableObject()
		{
			assert(false && "ERROR: Committed D3D12 resources cannot be deleted to free residency!");
			std::unreachable();
		}
	}
}