module;
#include <vector>
#include <span>
#include <cassert>
#include <optional>

module Brawler.D3D12.AliasedGPUMemoryManager;
import Brawler.D3D12.TransientGPUResourceAliasTracker;
import Brawler.D3D12.I_GPUResource;

namespace Brawler
{
	namespace D3D12
	{
		AliasedGPUMemoryManager::AliasedGPUMemoryManager(const TransientGPUResourceAliasTracker& aliasTracker) :
			mAliasTrackerPtr(&aliasTracker),
			mStateArr()
		{
			mStateArr.resize(aliasTracker.GetAliasGroupCount());
		}

		const I_GPUResource* AliasedGPUMemoryManager::ActivateGPUResource(const I_GPUResource& activatedResource)
		{
			assert(mAliasTrackerPtr != nullptr);
			const std::optional<std::uint32_t> resourceAliasGroupID{ mAliasTrackerPtr->GetAliasGroupID(activatedResource) };

			// If the provided resource does not have an alias group ID, then it must be a persistent 
			// resource, since the TransientGPUResourceAliasTracker provided at creation did not have 
			// it. In that case, we never need an aliasing barrier to use it.
			if (!resourceAliasGroupID.has_value())
			{
				assert(activatedResource.GetGPUResourceLifetimeType() == GPUResourceLifetimeType::PERSISTENT);
				return nullptr;
			}

			assert(*resourceAliasGroupID < mStateArr.size() && "ERROR: A transient I_GPUResource instance was provided an invalid alias group ID by a TransientGPUResourceAliasTracker instance!");
			AliasedGPUMemoryState& memoryState{ mStateArr[*resourceAliasGroupID] };

			const I_GPUResource* const prevResource = memoryState.CurrentResourcePtr;

			if (prevResource == &activatedResource)
				return nullptr;

			memoryState.CurrentResourcePtr = &activatedResource;

			return prevResource;
		}
	}
}