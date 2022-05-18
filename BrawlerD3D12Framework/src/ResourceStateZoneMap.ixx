module;
#include <memory>
#include <vector>
#include <span>
#include <ranges>
#include "DxDef.h"

export module Brawler.D3D12.ResourceStateZoneMap;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.GPUResourceStateManagement;
import Brawler.D3D12.GPUResourceStateTracker;
import Brawler.D3D12.ResourceStateZone;

export namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		class I_RenderPass;

		class I_GPUResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class ResourceStateZoneMap
		{
		private:
			using ResourceStateZoneMapSegment = std::vector<ResourceStateZone>;

		public:
			explicit ResourceStateZoneMap(I_GPUResource& resource);

			ResourceStateZoneMap(const ResourceStateZoneMap& rhs) = delete;
			ResourceStateZoneMap& operator=(const ResourceStateZoneMap& rhs) = delete;

			ResourceStateZoneMap(ResourceStateZoneMap&& rhs) noexcept = default;
			ResourceStateZoneMap& operator=(ResourceStateZoneMap&& rhs) noexcept = default;

			void AddResourceStateZone(const ResourceStateZone& stateZone);
			void AddResourceStateZones(std::span<const ResourceStateZone> stateZoneSpan);

			void OptimizeResourceStateZones();

			void AddStateDecayBarrier();

			GPUResourceEventManager CreateGPUResourceEventManager();

		private:
			void CheckSegmentForImplicitTransition(ResourceStateZoneMapSegment& mapSegment) const;

		private:
			std::vector<ResourceStateZoneMapSegment> mMapSegmentArr;
			I_GPUResource* mResourcePtr;
			D3D12_RESOURCE_STATES mResourceStartState;
		};
	}
}