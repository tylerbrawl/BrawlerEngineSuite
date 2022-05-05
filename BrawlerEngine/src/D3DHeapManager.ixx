module;
#include <concepts>
#include <memory>
#include <unordered_map>
#include <optional>
#include <limits>
#include <functional>
#include "DxDef.h"

export module Brawler.D3DHeapManager;
import Brawler.ResourceCreationInfo;
import Brawler.D3DHeapPool;
import Brawler.JobGroup;
import Util.Engine;
import Brawler.I_GPUResource;

export namespace Brawler
{
	class Renderer;
}

export namespace Brawler
{
	class D3DHeapManager
	{
	private:
		friend class Renderer;

	public:
		D3DHeapManager();

		D3DHeapManager(const D3DHeapManager& rhs) = delete;
		D3DHeapManager& operator=(const D3DHeapManager& rhs) = delete;

		D3DHeapManager(D3DHeapManager&& rhs) noexcept = default;
		D3DHeapManager& operator=(D3DHeapManager&& rhs) noexcept = default;

	private:
		template <typename T, typename... Args>
			requires std::derived_from<T, I_GPUResource>
		std::unique_ptr<T> CreateResource(const Brawler::D3DResourceAccessMode accessMode, const D3D12_RESOURCE_STATES initialState, Args&&... args);

	private:
		std::unordered_map<AllowedD3DResourceType, D3DHeapPool> mHeapPoolMap;
	};
}

// --------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T, typename... Args>
		requires std::derived_from<T, I_GPUResource>
	std::unique_ptr<T> D3DHeapManager::CreateResource(const Brawler::D3DResourceAccessMode accessMode, const D3D12_RESOURCE_STATES initialState, Args&&... args)
	{
		std::unique_ptr<T> resource{ std::make_unique<T>(std::forward<Args>(args)...) };
		resource->mProperlyInitialized = true;

		ResourceCreationInfo creationInfo{};
		creationInfo.AccessMode = accessMode;

		// Get information regarding the resource which is to be created.
		Brawler::D3D12_RESOURCE_DESC resourceDesc{ resource->GetResourceDescription() };
		const AllowedD3DResourceType resourceType{ Brawler::GetAllowedResourceTypeFromResourceDescription(resourceDesc) };

		// First, try to get the small alignment, if it is possible. (Buffers, render targets,
		// and depth/stencil buffers do not have a small alignment.)
		D3D12_RESOURCE_ALLOCATION_INFO allocInfo{};
		const D3DHeapInfo heapInfo{ Brawler::GetHeapInfoFromAllowedResourceType(resourceType) };

		{
			D3D12_RESOURCE_ALLOCATION_INFO1 tempAllocInfo{};

			if (heapInfo.SmallAlignment)
			{
				resourceDesc.Alignment = *(heapInfo.SmallAlignment);
				allocInfo = Util::Engine::GetD3D12Device().GetResourceAllocationInfo2(0, 1, &resourceDesc, &tempAllocInfo);

				// If allocInfo.SizeInBytes == UINT_MAX, then small alignment is not possible, and we
				// must resort to default alignment.
				if (allocInfo.SizeInBytes == std::numeric_limits<std::uint32_t>::max())
				{
					resourceDesc.Alignment = heapInfo.DefaultAlignment;
					allocInfo = Util::Engine::GetD3D12Device().GetResourceAllocationInfo2(0, 1, &resourceDesc, &tempAllocInfo);
				}
			}
			else
			{
				resourceDesc.Alignment = heapInfo.DefaultAlignment;
				allocInfo = Util::Engine::GetD3D12Device().GetResourceAllocationInfo2(0, 1, &resourceDesc, &tempAllocInfo);
			}
		}

		creationInfo.AllocationInfo = std::move(allocInfo);
		creationInfo.ResourceDesc = std::move(resourceDesc);
		creationInfo.InitialState = initialState;
		creationInfo.Resource = resource.get();

		// Prevent buffers from ever having an optimized clear value. This is required by the
		// D3D12 runtime.
		if (resourceType != AllowedD3DResourceType::BUFFERS)
			creationInfo.OptimizedClearValue = resource->GetOptimizedClearValue();

		// Create the resource.
		mHeapPoolMap.at(resourceType).CreatePlacedResource(creationInfo);
		
		return resource;
	}
}