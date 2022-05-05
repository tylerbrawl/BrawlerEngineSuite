module;
#include <span>
#include <cassert>
#include <array>
#include <ranges>
#include "DxDef.h"

export module Brawler.D3D12.I_GPUResourceHeapManager;
import Brawler.D3D12.GPUResourceLifetimeType;
import Util.Engine;

// More MSVC modules jank...
import Brawler.D3D12.GPUResourceHeap;

export namespace Brawler
{
	namespace D3D12
	{
		template <GPUResourceLifetimeType LifetimeType>
		class I_GPUResourceHeapManager
		{
		protected:
			I_GPUResourceHeapManager() = default;

		public:
			virtual ~I_GPUResourceHeapManager() = default;

			I_GPUResourceHeapManager(const I_GPUResourceHeapManager& rhs) = delete;
			I_GPUResourceHeapManager& operator=(const I_GPUResourceHeapManager& rhs) = delete;

			I_GPUResourceHeapManager(I_GPUResourceHeapManager&& rhs) noexcept = default;
			I_GPUResourceHeapManager& operator=(I_GPUResourceHeapManager&& rhs) noexcept = default;

			virtual void Initialize() = 0;

			HRESULT AllocateResource(I_GPUResource& resource, const D3D12_HEAP_TYPE heapType);
			HRESULT AllocateAliasedResources(const std::span<I_GPUResource* const> aliasedResources, const D3D12_HEAP_TYPE heapType);

			bool CanResourcesAlias(const std::span<I_GPUResource* const> aliasedResources) const;

		protected:
			virtual HRESULT AllocateAliasedResourcesIMPL(const GPUResourceAllocationInfo& allocationInfo, const D3D12_HEAP_TYPE heapType) = 0;
			virtual bool CanResourcesAliasIMPL(const std::span<I_GPUResource* const> aliasedResources) const = 0;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		D3D12_RESOURCE_ALLOCATION_INFO GetResourceAllocationInfo(Brawler::D3D12::I_GPUResource& resource)
		{
			// Create a new Brawler::D3D12_RESOURCE_DESC so that we can try different alignment values.
			Brawler::D3D12_RESOURCE_DESC resourceDesc{ resource.GetResourceDescription() };

			// We don't actually need this, but the API expects us to include one.
			D3D12_RESOURCE_ALLOCATION_INFO1 allocationInfo1{};

			// If this is a buffer resource, then the alignment must be D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT.
			if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER)
			{
				resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

				const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo{ Util::Engine::GetD3D12Device().GetResourceAllocationInfo2(0, 1, &resourceDesc, &allocationInfo1) };
				assert(allocationInfo.SizeInBytes != std::numeric_limits<std::uint64_t>::max() && "ERROR: The D3D12_RESOURCE_ALLOCATION_INFO for a buffer could not be obtained!");

				resource.SetResourceDescription(std::move(resourceDesc));
				return allocationInfo;
			}

			// Otherwise, it is possible that we can get away with small alignment. If the texture is
			// an MSAA texture (which we basically disallow at the time of writing, but we'll check for it 
			// anyways, in case things change in the future), then the small alignment is 64KB and the
			// default alignment is 4MB. Otherwise, the small alignment is 4KB and the default alignment
			// is 64KB.
			//
			// There is actually another check which we need to do. Specifically, if the resource is either
			// a render target or a depth/stencil texture, then its alignment can never be small.
			const bool isMSAATexture = (resourceDesc.SampleDesc.Count > 1);
			D3D12_RESOURCE_ALLOCATION_INFO allocationInfo{};

			if ((resourceDesc.Flags & (D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) == 0)
			{
				// First, try the small alignment.
				resourceDesc.Alignment = (isMSAATexture ? D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT);
				allocationInfo = Util::Engine::GetD3D12Device().GetResourceAllocationInfo2(0, 1, &resourceDesc, &allocationInfo1);

				if (allocationInfo.SizeInBytes != std::numeric_limits<std::uint64_t>::max())
				{
					// We were able to get away with small alignment.
					resource.SetResourceDescription(std::move(resourceDesc));
					return allocationInfo;
				}
			}

			// If that fails, then we need to use the default alignment.
			resourceDesc.Alignment = (isMSAATexture ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
			allocationInfo = Util::Engine::GetD3D12Device().GetResourceAllocationInfo2(0, 1, &resourceDesc, &allocationInfo1);

			assert(allocationInfo.SizeInBytes != std::numeric_limits<std::uint64_t>::max() && "ERROR: The D3D12_RESOURCE_ALLOCATION_INFO for a texture could not be obtained!");

			resource.SetResourceDescription(std::move(resourceDesc));
			return allocationInfo;
		}

		/// <summary>
		/// Since aliased resources are stored in the same location in memory, we need to make sure that
		/// the memory region used is large enough to hold the largest resource. Likewise, the alignment
		/// of the memory region needs to be as large as the largest required alignment. (This works because
		/// each larger alignment is a multiple of the smaller alignment values.)
		/// 
		/// Given this information, this function returns the required D3D12_RESOURCE_ALLOCATION_INFO to
		/// guarantee that the memory region allocated can hold at least one of the resources defined by
		/// aliasedResources at a time.
		/// </summary>
		/// <param name="aliasedResources">
		/// - The resources which are to be aliased with each other. They cannot be used simultaneously,
		///   since they are meant to share the same location in memory.
		/// </param>
		/// <returns>
		/// The function returns the required D3D12_RESOURCE_ALLOCATION_INFO to guarantee that the memory 
		/// region allocated can hold at least one of the resources defined by aliasedResources at a time.
		/// </returns>
		D3D12_RESOURCE_ALLOCATION_INFO GetRequiredAllocationInfo(const std::span<Brawler::D3D12::I_GPUResource* const> aliasedResources)
		{
			D3D12_RESOURCE_ALLOCATION_INFO largestAllocationInfo{
				.SizeInBytes = std::numeric_limits<std::uint64_t>::min(),
				.Alignment = std::numeric_limits<std::uint64_t>::min()
			};

			for (auto& resourcePtr : aliasedResources)
			{
				const D3D12_RESOURCE_ALLOCATION_INFO resourceAllocInfo{ GetResourceAllocationInfo(*resourcePtr) };

				if (resourceAllocInfo.Alignment > largestAllocationInfo.Alignment)
					largestAllocationInfo.Alignment = resourceAllocInfo.Alignment;

				if (resourceAllocInfo.SizeInBytes > largestAllocationInfo.SizeInBytes)
					largestAllocationInfo.SizeInBytes = resourceAllocInfo.SizeInBytes;
			}

			return largestAllocationInfo;
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <GPUResourceLifetimeType LifetimeType>
		HRESULT I_GPUResourceHeapManager<LifetimeType>::AllocateResource(I_GPUResource& resource, const D3D12_HEAP_TYPE heapType)
		{
			std::vector<I_GPUResource*> resourcePtrArr{ &resource };
			return AllocateAliasedResources(std::span<I_GPUResource*>{ resourcePtrArr }, heapType);
		}

		template <GPUResourceLifetimeType LifetimeType>
		HRESULT I_GPUResourceHeapManager<LifetimeType>::AllocateAliasedResources(const std::span<I_GPUResource* const> aliasedResources, const D3D12_HEAP_TYPE heapType)
		{
			if (aliasedResources.empty() || !CanResourcesAlias(aliasedResources)) [[unlikely]]
				return E_INVALIDARG;

			const GPUResourceAllocationInfo allocationInfo{
				.AliasedResources{ aliasedResources },
				.AllocationInfo{ GetRequiredAllocationInfo(aliasedResources) }
			};
			return AllocateAliasedResourcesIMPL(allocationInfo, heapType);
		}

		template <GPUResourceLifetimeType LifetimeType>
		bool I_GPUResourceHeapManager<LifetimeType>::CanResourcesAlias(const std::span<I_GPUResource* const> aliasedResources) const
		{
			// Regardless of the Resource Heap Tier support for a device, if the resources do not request
			// to be placed into the same type of heap, then they cannot alias each other.
			if (aliasedResources.empty())
				return true;

			const D3D12_HEAP_TYPE requiredHeapType = aliasedResources[0]->GetHeapType();

			for (const auto resourcePtr : aliasedResources | std::views::drop(1))
			{
				if (resourcePtr->GetHeapType() != requiredHeapType)
					return false;
			}

			return CanResourcesAliasIMPL(aliasedResources);
		}
	}
}