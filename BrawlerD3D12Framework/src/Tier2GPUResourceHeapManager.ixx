module;
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.Tier2GPUResourceHeapManager;
import Brawler.D3D12.I_GPUResourceHeapManager;
import Brawler.D3D12.GPUResourceHeapPool;
import Brawler.D3D12.GPUResourceLifetimeType;

export namespace Brawler
{
	namespace D3D12
	{
		template <GPUResourceLifetimeType LifetimeType>
		class Tier2GPUResourceHeapManager final : public I_GPUResourceHeapManager<LifetimeType>
		{
		public:
			Tier2GPUResourceHeapManager() = default;

			Tier2GPUResourceHeapManager(const Tier2GPUResourceHeapManager& rhs) = delete;
			Tier2GPUResourceHeapManager& operator=(const Tier2GPUResourceHeapManager& rhs) = delete;

			Tier2GPUResourceHeapManager(Tier2GPUResourceHeapManager&& rhs) noexcept = default;
			Tier2GPUResourceHeapManager& operator=(Tier2GPUResourceHeapManager&& rhs) noexcept = default;

			void Initialize() override;

		protected:
			HRESULT AllocateAliasedResourcesIMPL(const GPUResourceAllocationInfo& allocationInfo, const D3D12_HEAP_TYPE heapType) override;
			bool CanResourcesAliasIMPL(const std::span<I_GPUResource* const> aliasedResources) const override;

		private:
			GPUResourceHeapPool<D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT, LifetimeType> mDefaultPool;
			GPUResourceHeapPool<D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD, LifetimeType> mUploadPool;
			GPUResourceHeapPool<D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK, LifetimeType> mReadbackPool;
		};
	}
}

// ------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUResourceLifetimeType LifetimeType>
		void Tier2GPUResourceHeapManager<LifetimeType>::Initialize()
		{
			static constexpr D3D12_HEAP_FLAGS DEFAULT_FLAGS = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES | D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;

			mDefaultPool.Initialize(DEFAULT_FLAGS);
			mUploadPool.Initialize(DEFAULT_FLAGS);
			mReadbackPool.Initialize(DEFAULT_FLAGS);
		}

		template <GPUResourceLifetimeType LifetimeType>
		HRESULT Tier2GPUResourceHeapManager<LifetimeType>::AllocateAliasedResourcesIMPL(const GPUResourceAllocationInfo& allocationInfo, const D3D12_HEAP_TYPE heapType)
		{
			switch (heapType)
			{
			case D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT:
				return mDefaultPool.AllocateResources(allocationInfo);

			case D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD:
				return mUploadPool.AllocateResources(allocationInfo);

			case D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK:
				return mReadbackPool.AllocateResources(allocationInfo);

			default:
				return E_INVALIDARG;
			}
		}

		template <GPUResourceLifetimeType LifetimeType>
		bool Tier2GPUResourceHeapManager<LifetimeType>::CanResourcesAliasIMPL(const std::span<I_GPUResource* const> aliasedResources) const
		{
			// Devices with D3D12_RESOURCE_HEAP_TIER_2 support allow any type of resource to be placed
			// into the same heap, so any resources can thus alias with each other.

			return true;
		}
	}
}