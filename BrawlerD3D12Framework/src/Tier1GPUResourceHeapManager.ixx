module;
#include <array>
#include <span>
#include <ranges>
#include "DxDef.h"

export module Brawler.D3D12.Tier1GPUResourceHeapManager;
import Brawler.D3D12.I_GPUResourceHeapManager;
import Brawler.D3D12.GPUResourceHeapPool;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceLifetimeType;

export namespace Brawler
{
	namespace D3D12
	{
		/// <summary>
		/// This derived I_GPUResourceHeapManager class is meant for devices with only
		/// D3D12_RESOURCE_HEAP_TIER_1 support. It splits the heaps into three categories:
		/// buffers, textures which are neither render targets nor depth/stencil textures,
		/// and textures which are render targets or depth/stencil textures.
		/// 
		/// This tends to lead to the creation of many more heaps. On the other hand, this
		/// also means that there will be less contention on the locks used to implement
		/// heap management. We may want to just use this version for all devices, if we
		/// find that performance improves significantly.
		/// </summary>
		template <GPUResourceLifetimeType LifetimeType>
		class Tier1GPUResourceHeapManager final : public I_GPUResourceHeapManager<LifetimeType>
		{
		private:
			struct HeapPoolLocker
			{
				GPUResourceHeapPool<D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT, LifetimeType> DefaultPool;
				GPUResourceHeapPool<D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD, LifetimeType> UploadPool;
				GPUResourceHeapPool<D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK, LifetimeType> ReadbackPool;

				void Initialize(const D3D12_HEAP_FLAGS heapFlags);
			};

			enum class ResourceType
			{
				BUFFERS,
				NON_RT_DS_TEXTURES,
				RT_DS_TEXTURES,

				COUNT_OR_ERROR
			};

		public:
			Tier1GPUResourceHeapManager() = default;

			Tier1GPUResourceHeapManager(const Tier1GPUResourceHeapManager& rhs) = delete;
			Tier1GPUResourceHeapManager& operator=(const Tier1GPUResourceHeapManager& rhs) = delete;

			Tier1GPUResourceHeapManager(Tier1GPUResourceHeapManager&& rhs) noexcept = default;
			Tier1GPUResourceHeapManager& operator=(Tier1GPUResourceHeapManager&& rhs) noexcept = default;

			void Initialize() override;

		protected:
			HRESULT AllocateAliasedResourcesIMPL(const GPUResourceAllocationInfo& allocationInfo, const D3D12_HEAP_TYPE heapType) override;
			bool CanResourcesAliasIMPL(const std::span<I_GPUResource* const> aliasedResources) const override;

		private:
			ResourceType GetResourceType(const I_GPUResource& resource) const;

		private:
			std::array<HeapPoolLocker, std::to_underlying(ResourceType::COUNT_OR_ERROR)> mPoolLockerArr;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUResourceLifetimeType LifetimeType>
		void Tier1GPUResourceHeapManager<LifetimeType>::HeapPoolLocker::Initialize(const D3D12_HEAP_FLAGS heapFlags)
		{
			DefaultPool.Initialize(heapFlags);
			UploadPool.Initialize(heapFlags);
			ReadbackPool.Initialize(heapFlags);
		}

		template <GPUResourceLifetimeType LifetimeType>
		void Tier1GPUResourceHeapManager<LifetimeType>::Initialize()
		{
			mPoolLockerArr[std::to_underlying(ResourceType::BUFFERS)].Initialize(D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS | D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_CREATE_NOT_ZEROED);
			mPoolLockerArr[std::to_underlying(ResourceType::NON_RT_DS_TEXTURES)].Initialize(D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES | D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_CREATE_NOT_ZEROED);
			mPoolLockerArr[std::to_underlying(ResourceType::RT_DS_TEXTURES)].Initialize(D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES | D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_CREATE_NOT_ZEROED);
		}

		template <GPUResourceLifetimeType LifetimeType>
		HRESULT Tier1GPUResourceHeapManager<LifetimeType>::AllocateAliasedResourcesIMPL(const GPUResourceAllocationInfo& allocationInfo, const D3D12_HEAP_TYPE heapType)
		{
			// The base class already ensures that the resources can alias, so we just need to get the
			// type for the first one.
			const ResourceType resourceType{ GetResourceType(*(allocationInfo.AliasedResources[0])) };
			HeapPoolLocker& poolLocker{ mPoolLockerArr[std::to_underlying(resourceType)] };

			switch (heapType)
			{
			case D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT:
				return poolLocker.DefaultPool.AllocateResources(allocationInfo);

			case D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD:
				return poolLocker.UploadPool.AllocateResources(allocationInfo);

			case D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK:
				return poolLocker.ReadbackPool.AllocateResources(allocationInfo);

			default:
				return E_INVALIDARG;
			}
		}

		template <GPUResourceLifetimeType LifetimeType>
		bool Tier1GPUResourceHeapManager<LifetimeType>::CanResourcesAliasIMPL(const std::span<I_GPUResource* const> aliasedResources) const
		{
			if (aliasedResources.size() <= 1)
				return true;

			const ResourceType requiredType = GetResourceType(*(aliasedResources[0]));
			const std::span<I_GPUResource* const> remainingResourcesSpan{ aliasedResources.subspan(1) };

			for (const auto& resourcePtr : remainingResourcesSpan)
			{
				if (GetResourceType(*resourcePtr) != requiredType)
					return false;
			}

			return true;
		}

		template <GPUResourceLifetimeType LifetimeType>
		Tier1GPUResourceHeapManager<LifetimeType>::ResourceType Tier1GPUResourceHeapManager<LifetimeType>::GetResourceType(const I_GPUResource& resource) const
		{
			const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ resource.GetResourceDescription() };

			if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER)
				return ResourceType::BUFFERS;

			const bool isRTOrDSTexture = ((resourceDesc.Flags & (D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) != 0);
			return (isRTOrDSTexture ? ResourceType::RT_DS_TEXTURES : ResourceType::NON_RT_DS_TEXTURES);
		}
	}
}