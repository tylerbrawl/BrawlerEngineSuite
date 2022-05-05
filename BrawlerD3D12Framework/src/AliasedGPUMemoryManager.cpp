module;
#include <vector>
#include <unordered_map>
#include <span>
#include <cassert>

module Brawler.D3D12.AliasedGPUMemoryManager;
import Brawler.D3D12.TransientGPUResourceAliasTracker;
import Brawler.D3D12.I_GPUResource;

namespace Brawler
{
	namespace D3D12
	{
		AliasedGPUMemoryManager::AliasedGPUMemoryManager(const TransientGPUResourceAliasTracker& aliasTracker) :
			mStateArr(),
			mResourceMemoryStateMap()
		{
			const std::span<const std::vector<I_GPUResource*>> aliasableResourcesSpan{ aliasTracker.GetAliasableResources() };

			mStateArr.resize(aliasableResourcesSpan.size());

			for (std::size_t stateIndex = 0; stateIndex < mStateArr.size(); ++stateIndex)
			{
				for (const auto resourcePtr : aliasableResourcesSpan[stateIndex])
					mResourceMemoryStateMap[resourcePtr] = &(mStateArr[stateIndex]);
			}
		}

		const I_GPUResource* AliasedGPUMemoryManager::ActivateGPUResource(const I_GPUResource& activatedResource)
		{
			// If the provided resource is not in the map, then it must be a transient resource, since
			// the TransientGPUResourceAliasTracker provided at creation did not have it. In that case,
			// we never need an aliasing barrier to use it.
			if (!mResourceMemoryStateMap.contains(&activatedResource))
			{
				assert(activatedResource.GetGPUResourceLifetimeType() == GPUResourceLifetimeType::PERSISTENT);
				return nullptr;
			}
			
			AliasedGPUMemoryState& memoryState{ *(mResourceMemoryStateMap.at(&activatedResource)) };

			const I_GPUResource* const prevResource = memoryState.CurrentResourcePtr;

			if (prevResource == &activatedResource)
				return nullptr;

			memoryState.CurrentResourcePtr = &activatedResource;

			return prevResource;
		}
	}
}