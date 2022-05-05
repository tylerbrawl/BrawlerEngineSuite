module;
#include <span>
#include <unordered_map>
#include <cassert>
#include <array>
#include "DxDef.h"

module Brawler.D3D12.TransientGPUResourceAliasTracker;
import Brawler.D3D12.I_GPUResource;
import Brawler.SortedVector;
import Util.Engine;

namespace
{
	std::uint64_t GetGPUResourceSize(const Brawler::D3D12::I_GPUResource& resource)
	{
		D3D12_RESOURCE_ALLOCATION_INFO1 allocInfo1{};
		const D3D12_RESOURCE_ALLOCATION_INFO allocInfo{ Util::Engine::GetD3D12Device().GetResourceAllocationInfo2(0, 1, &(resource.GetResourceDescription()), &allocInfo1) };

		return allocInfo.SizeInBytes;
	}
}

namespace Brawler
{
	namespace D3D12
	{
		bool TransientGPUResourceAliasTracker::TransientGPUResourceInfo::Overlaps(const TransientGPUResourceInfo& otherInfo) const
		{
			assert(ResourcePtr != otherInfo.ResourcePtr && "ERROR: An attempt was made to check if a transient I_GPUResource instance overlaps with itself!");

			return ((FirstBundleUsage <= otherInfo.FirstBundleUsage && otherInfo.FirstBundleUsage <= LastBundleUsage) ||
				(otherInfo.FirstBundleUsage <= FirstBundleUsage && FirstBundleUsage <= otherInfo.LastBundleUsage));
		}

		void TransientGPUResourceAliasTracker::SetTransientGPUResourceHeapManager(const I_GPUResourceHeapManager<GPUResourceLifetimeType::TRANSIENT>& resourceHeapManager)
		{
			mResourceHeapManagerPtr = &resourceHeapManager;
		}

		void TransientGPUResourceAliasTracker::AddTransientResourceDependencyForBundle(const std::uint32_t bundleID, I_GPUResource& transientResourceDependency)
		{
			assert(transientResourceDependency.GetGPUResourceLifetimeType() == GPUResourceLifetimeType::TRANSIENT && "ERROR: An attempt was made to try and alias a persistent I_GPUResource with transient I_GPUResources!");

			I_GPUResource* const resourcePtr = &transientResourceDependency;

			if (mResourceLifetimeMap.contains(resourcePtr))
				mResourceLifetimeMap[resourcePtr].LastBundleUsage = std::max(mResourceLifetimeMap[resourcePtr].LastBundleUsage, bundleID);
			else
				mResourceLifetimeMap[resourcePtr] = TransientGPUResourceInfo{
					.ResourcePtr = resourcePtr,
					.ResourceSize = GetGPUResourceSize(transientResourceDependency),
					.FirstBundleUsage = bundleID,
					.LastBundleUsage = bundleID
				};
		}

		void TransientGPUResourceAliasTracker::CalculateAliasableResources()
		{
			// I'm not sure if there is a reliable algorithm for determining the best way to alias
			// resources in order to minimize total memory usage. However, this heuristic process
			// seems to produce good results in my arbitrary test scenarios:
			//
			//   1. Choose the resource with the largest total memory consumption.
			//   2. Find which resources can alias with this resource. Prefer larger resources over
			//      smaller ones.
			//   3. Repeat this process until all resources have been assigned.
			//
			// The problem lies in doing this efficiently. Remember: We are doing this once per frame.
			// All we can really do at this point is hope that it runs well; if it doesn't, then we
			// can always go back and optimize the process.

			static constexpr auto SORT_TRANSIENT_GPU_RESOURCE_INFO_LAMBDA = [] (const TransientGPUResourceInfo* lhs, const TransientGPUResourceInfo* rhs)
			{
				// Order the resource information structures from largest corresponding resource to
				// smallest corresponding resource.

				return (lhs->ResourceSize > rhs->ResourceSize);
			};

			Brawler::SortedVector<const TransientGPUResourceInfo*, decltype(SORT_TRANSIENT_GPU_RESOURCE_INFO_LAMBDA)> sortedResourceInfoArr{};
			sortedResourceInfoArr.Reserve(mResourceLifetimeMap.size());

			for (const auto& [resourcePtr, resourceInfo] : mResourceLifetimeMap)
				sortedResourceInfoArr.Insert(&resourceInfo);

			while (!sortedResourceInfoArr.Empty())
			{
				std::vector<const TransientGPUResourceInfo*> aliasableResourceInfoArr{};
				sortedResourceInfoArr.ForEach([this, &aliasableResourceInfoArr] (const TransientGPUResourceInfo* const& currInfo)
				{
					// We can skip all of this logic if aliasableResourceInfoArr is currently empty.
					if (aliasableResourceInfoArr.empty())
					{
						aliasableResourceInfoArr.push_back(currInfo);
						return;
					}

					const bool canCurrentResourceAliasBeforeGPUUse = currInfo->ResourcePtr->CanAliasBeforeUseOnGPU();
					const bool canCurrentResourceAliasAfterGPUUse = currInfo->ResourcePtr->CanAliasAfterUseOnGPU();
						
					// Check to make sure that the resource corresponding to currInfo does not overlap with
					// any of the resources in aliasableResourceInfoArr.
					for (const auto aliasedResourceInfoPtr : aliasableResourceInfoArr)
					{
						if (aliasedResourceInfoPtr->Overlaps(*currInfo))
							return;

						// If the resource does not allow aliasing before it is used, then do not alias
						// it if any aliasedResourceInfoPtr has an earlier use. We also need to check that 
						// the resource allows aliasing after it is used.
						if ((!canCurrentResourceAliasBeforeGPUUse && (aliasedResourceInfoPtr->FirstBundleUsage <= currInfo->FirstBundleUsage)) ||
							(!canCurrentResourceAliasAfterGPUUse && (currInfo->LastBundleUsage <= aliasedResourceInfoPtr->LastBundleUsage)))
							return;

						// Now, we need to do the same thing again. This time, however, we need to check it for
						// aliasedResourceInfoPtr.
						if ((!aliasedResourceInfoPtr->ResourcePtr->CanAliasBeforeUseOnGPU() && (currInfo->FirstBundleUsage <= aliasedResourceInfoPtr->FirstBundleUsage)) ||
							(!aliasedResourceInfoPtr->ResourcePtr->CanAliasAfterUseOnGPU() && (aliasedResourceInfoPtr->LastBundleUsage <= currInfo->LastBundleUsage)))
							return;
					}

					// If the resources do not overlap with each other, then they should theoretically
					// be able to alias each other. In practice, however, this is not necessarily the
					// case. Specifically, on hardware with only Resource Heap Tier 1 support, heaps
					// are only allowed to contain certain types of resources.
					//
					// So, before we can assume that the resources are perfectly aliasable with each
					// other, we need to verify that the hardware supports it.
					std::vector<I_GPUResource*> resourcesToAlias{};
					resourcesToAlias.reserve(aliasableResourceInfoArr.size() + 1);

					for (const auto aliasableResourceInfoPtr : aliasableResourceInfoArr)
						resourcesToAlias.push_back(aliasableResourceInfoPtr->ResourcePtr);

					resourcesToAlias.push_back(currInfo->ResourcePtr);

					if (!mResourceHeapManagerPtr->CanResourcesAlias(resourcesToAlias))
						return;

					// At this point, we know that we really can alias the resources.
					aliasableResourceInfoArr.push_back(currInfo);
				});

				std::vector<I_GPUResource*> aliasedResourcePtrArr{};
				aliasedResourcePtrArr.reserve(aliasableResourceInfoArr.size());

				// Remove each processed TransientGPUResourceInfo from the list of aliasable resource infos
				// and add its ResourcePtr to aliasedResourcePtrArr.
				for (const auto aliasedResourceInfoPtr : aliasableResourceInfoArr)
				{
					sortedResourceInfoArr.Remove(aliasedResourceInfoPtr);
					aliasedResourcePtrArr.push_back(aliasedResourceInfoPtr->ResourcePtr);
				}

				// Finally, aliasedResourcePtrArr contains the set of all resources which are aliasable
				// with the resource which we started with.
				mAliasableResourcesArr.push_back(std::move(aliasedResourcePtrArr));
			}
		}

		std::span<const std::vector<I_GPUResource*>> TransientGPUResourceAliasTracker::GetAliasableResources() const
		{
			return std::span<const std::vector<I_GPUResource*>>{ mAliasableResourcesArr };
		}
	}
}