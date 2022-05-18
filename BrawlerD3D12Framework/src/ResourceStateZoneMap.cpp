module;
#include <memory>
#include <vector>
#include <span>
#include <ranges>
#include "DxDef.h"

module Brawler.D3D12.ResourceStateZoneMap;
import Brawler.D3D12.I_GPUResource;
import Util.D3D12;
import Brawler.D3D12.ResourceStateZoneOptimizer;

namespace Brawler
{
	namespace D3D12
	{
		ResourceStateZoneMap::ResourceStateZoneMap(I_GPUResource& resource) :
			mMapSegmentArr(),
			mResourcePtr(&resource),
			mResourceStartState(resource.GetCurrentResourceState())
		{
			mMapSegmentArr.emplace_back();
		}

		void ResourceStateZoneMap::AddResourceStateZone(const ResourceStateZone& stateZone)
		{
			// If the specified ResourceStateZone is requesting a transition to the
			// D3D12_RESOURCE_STATE_COMMON state, then we effectively have a state decay
			// here.
			const bool transitionsToCommonState = (!stateZone.IsNull() && *(stateZone.RequiredState) == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);

			mMapSegmentArr.back().push_back(stateZone);

			if (transitionsToCommonState)
				AddStateDecayBarrier();
		}

		void ResourceStateZoneMap::AddResourceStateZones(std::span<const ResourceStateZone> stateZoneSpan)
		{
			for (const auto& stateZone : stateZoneSpan)
				AddResourceStateZone(stateZone);
		}

		void ResourceStateZoneMap::OptimizeResourceStateZones()
		{
			// First, check for implicit resource state promotions.
			{
				if (mResourceStartState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON)
					CheckSegmentForImplicitTransition(mMapSegmentArr[0]);

				for (auto& mapSegment : mMapSegmentArr | std::views::drop(1))
					CheckSegmentForImplicitTransition(mapSegment);
			}

			ResourceStateZoneOptimizer zoneOptimizer{ *mResourcePtr };

			for (auto& mapSegment : mMapSegmentArr | std::views::take(mMapSegmentArr.size() - 1))
			{
				for (auto& stateZone : mapSegment)
					zoneOptimizer.ProcessResourceStateZone(stateZone);

				zoneOptimizer.OnStateDecayBarrier();
			}

			for (auto& stateZone : mMapSegmentArr.back())
				zoneOptimizer.ProcessResourceStateZone(stateZone);

			const std::span<ResourceStateZone* const> stateZonesToDeleteSpan{ zoneOptimizer.GetResourceStateZonesToDelete() };
			std::size_t numStateZonesDeleted = 0;

			for (auto& mapSegment : mMapSegmentArr)
			{
				for (auto& stateZone : mapSegment)
				{
					if (std::ranges::find(stateZonesToDeleteSpan, &stateZone) != stateZonesToDeleteSpan.end())
					{
						stateZone.IsDeleted = true;
						++numStateZonesDeleted;
					}
				}
			}

			assert(numStateZonesDeleted == stateZonesToDeleteSpan.size());
		}

		void ResourceStateZoneMap::AddStateDecayBarrier()
		{
			mMapSegmentArr.emplace_back();
		}

		GPUResourceEventManager ResourceStateZoneMap::CreateGPUResourceEventManager()
		{
			GPUResourceStateTracker stateTracker{ *mResourcePtr };

			for (const auto& mapSegment : mMapSegmentArr | std::views::take(mMapSegmentArr.size() - 1))
			{
				for (const auto& stateZone : mapSegment | std::views::filter([](const ResourceStateZone& stateZone){ return !(stateZone.IsDeleted); }))
					stateTracker.AddNextResourceStateZone(stateZone);

				stateTracker.OnStateDecayBarrier();
			}

			for (const auto& stateZone : mMapSegmentArr.back() | std::views::filter([] (const ResourceStateZone& stateZone) { return !(stateZone.IsDeleted); }))
				stateTracker.AddNextResourceStateZone(stateZone);

			return stateTracker.FinalizeStateTracking();
		}

		void ResourceStateZoneMap::CheckSegmentForImplicitTransition(ResourceStateZoneMapSegment& mapSegment) const
		{
			auto result = std::ranges::find_if(mapSegment, [] (const ResourceStateZone& stateZone) { return (!stateZone.IsNull()); });

			if (result != mapSegment.end())
			{
				ResourceStateZone& stateZone{ *result };
				stateZone.IsImplicitTransition = Util::D3D12::IsImplicitStateTransitionPossible(*mResourcePtr, *(stateZone.RequiredState));
			}
		}
	}
}