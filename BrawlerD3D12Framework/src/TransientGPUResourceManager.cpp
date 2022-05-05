module;
#include <span>
#include <memory>
#include <vector>
#include <cassert>

module Brawler.D3D12.TransientGPUResourceManager;
import Util.Engine;
import Brawler.D3D12.GPUCapabilities;
import Brawler.D3D12.Tier1GPUResourceHeapManager;
import Brawler.D3D12.Tier2GPUResourceHeapManager;

namespace Brawler
{
	namespace D3D12
	{
		void TransientGPUResourceManager::Initialize()
		{
			if (Util::Engine::GetGPUCapabilities().GPUResourceHeapTier == ResourceHeapTier::TIER_1)
				mTransientResourceHeapManager = std::make_unique<Tier1GPUResourceHeapManager<GPUResourceLifetimeType::TRANSIENT>>();
			else
				mTransientResourceHeapManager = std::make_unique<Tier2GPUResourceHeapManager<GPUResourceLifetimeType::TRANSIENT>>();

			mTransientResourceHeapManager->Initialize();
		}

		void TransientGPUResourceManager::AddTransientResources(const std::span<std::unique_ptr<I_GPUResource>> transientResourceSpan)
		{
			assert(mTransientResourceHeapManager != nullptr);

			mTransientResourceArr.reserve(mTransientResourceArr.size() + transientResourceSpan.size());
			
			for (auto&& resource : transientResourceSpan)
				mTransientResourceArr.push_back(std::move(resource));
		}

		void TransientGPUResourceManager::DeleteTransientResources()
		{
			mTransientResourceArr.clear();
		}

		I_GPUResourceHeapManager<GPUResourceLifetimeType::TRANSIENT>& TransientGPUResourceManager::GetGPUResourceHeapManager()
		{
			assert(mTransientResourceHeapManager != nullptr);
			return *(mTransientResourceHeapManager.get());
		}

		const I_GPUResourceHeapManager<GPUResourceLifetimeType::TRANSIENT>& TransientGPUResourceManager::GetGPUResourceHeapManager() const
		{
			assert(mTransientResourceHeapManager != nullptr);
			return *(mTransientResourceHeapManager.get());
		}
	}
}