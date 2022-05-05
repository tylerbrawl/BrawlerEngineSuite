module;
#include <shared_mutex>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.D3D12ResourceContainer;
import Brawler.ScopedSharedLock;
import Brawler.D3D12.I_GPUResource;

namespace Brawler
{
	namespace D3D12
	{
		D3D12ResourceContainer::D3D12ResourceContainer(I_GPUResource& owningResource) :
			mOwningResourcePtr(&owningResource),
			mD3DResource(nullptr),
			mCritSection()
		{}

		Brawler::D3D12Resource& D3D12ResourceContainer::GetD3D12Resource() const
		{
			Brawler::ScopedSharedReadLock<std::shared_mutex> readLock{ mCritSection };

			assert(mD3DResource != nullptr && "ERROR: An attempt was made to access the ID3D12Resource object of an I_GPUResource before it could ever be created! (This might be a *race condition*!)");
			return *(mD3DResource.Get());
		}

		I_GPUResource& D3D12ResourceContainer::GetGPUResource()
		{
			assert(mOwningResourcePtr != nullptr);
			return *mOwningResourcePtr;
		}

		const I_GPUResource& D3D12ResourceContainer::GetGPUResource() const
		{
			assert(mOwningResourcePtr != nullptr);
			return *mOwningResourcePtr;
		}

		void D3D12ResourceContainer::SetD3D12Resource(Microsoft::WRL::ComPtr<Brawler::D3D12Resource>&& d3dResource)
		{
			Brawler::ScopedSharedWriteLock<std::shared_mutex> writeLock{ mCritSection };

			mD3DResource = std::move(d3dResource);
		}
	}
}