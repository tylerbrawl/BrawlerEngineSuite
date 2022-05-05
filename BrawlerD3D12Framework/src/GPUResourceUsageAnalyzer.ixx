module;
#include <vector>
#include <span>
#include <optional>
#include <unordered_map>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceStateManagement:GPUResourceUsageAnalyzer;
import :GPUResourceEventManager;
import Brawler.D3D12.GPUCommandQueueType;

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource;
		class GPUExecutionModule;
		class ResourceStateZoneMap;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceUsageAnalyzer
		{
		public:
			explicit GPUResourceUsageAnalyzer(I_GPUResource& resource);

			GPUResourceUsageAnalyzer(const GPUResourceUsageAnalyzer& rhs) = delete;
			GPUResourceUsageAnalyzer& operator=(const GPUResourceUsageAnalyzer& rhs) = delete;

			GPUResourceUsageAnalyzer(GPUResourceUsageAnalyzer&& rhs) noexcept = default;
			GPUResourceUsageAnalyzer& operator=(GPUResourceUsageAnalyzer&& rhs) noexcept = default;

			void TraverseFrameGraph(const std::span<const GPUExecutionModule> executionModuleSpan);

			GPUResourceEventManager ExtractGPUResourceEventManager();

		private:
			void TrackResourceUsageInExecutionModule(const GPUExecutionModule& executionModule, ResourceStateZoneMap& stateZoneMap);

		private:
			I_GPUResource* mResourcePtr;
			D3D12_RESOURCE_STATES mCurrResourceState;
			GPUResourceEventManager mEventManager;
			bool mResourceAlwaysDecays;
		};
	}
}

