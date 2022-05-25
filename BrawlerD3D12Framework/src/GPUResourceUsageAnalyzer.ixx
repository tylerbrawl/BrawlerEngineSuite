module;
#include <vector>
#include <span>
#include <optional>
#include <unordered_map>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceUsageAnalyzer;
import Brawler.D3D12.GPUResourceEventCollection;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.GPUResourceStateTracker;
import Brawler.D3D12.I_GPUResource;

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

			GPUResourceEventCollection ExtractGPUResourceEventCollection();

		private:
			I_GPUResource* mResourcePtr;
			GPUResourceStateTracker mStateTracker;
		};
	}
}

