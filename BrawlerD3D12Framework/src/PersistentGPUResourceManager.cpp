module;
#include <memory>
#include "DxDef.h"

module Brawler.D3D12.PersistentGPUResourceManager;
import Util.Win32;
import Util.Engine;
import Brawler.D3D12.GPUCapabilities;
import Brawler.D3D12.Tier1GPUResourceHeapManager;
import Brawler.D3D12.Tier2GPUResourceHeapManager;
import Brawler.D3D12.I_GPUResource;

namespace Brawler
{
	namespace D3D12
	{
		void PersistentGPUResourceManager::Initialize()
		{
			CreateGPUResourceHeapManager();
		}

		HRESULT PersistentGPUResourceManager::AllocatePersistentGPUResource(I_GPUResource& resource)
		{
			return mHeapManager->AllocateResource(resource, resource.GetHeapType());
		}

		void PersistentGPUResourceManager::CreateGPUResourceHeapManager()
		{
			// Create the best possible GPUResourceHeapManager for this device.
			const GPUCapabilities& deviceCapabilities{ Util::Engine::GetGPUCapabilities() };

			if (deviceCapabilities.GPUResourceHeapTier == ResourceHeapTier::TIER_1)
				mHeapManager = std::make_unique<Tier1GPUResourceHeapManager<GPUResourceLifetimeType::PERSISTENT>>();
			else
				mHeapManager = std::make_unique<Tier2GPUResourceHeapManager<GPUResourceLifetimeType::PERSISTENT>>();

			mHeapManager->Initialize();
		}
	}
}