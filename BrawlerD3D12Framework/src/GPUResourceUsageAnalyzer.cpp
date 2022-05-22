module;
#include <cassert>
#include <span>
#include "DxDef.h"

module Brawler.D3D12.GPUResourceUsageAnalyzer;
import Brawler.D3D12.GPUResourceStateTracker;

namespace Brawler
{
	namespace D3D12
	{
		GPUResourceUsageAnalyzer::GPUResourceUsageAnalyzer(I_GPUResource& resource) :
			mResourcePtr(&resource),
			mStateTracker(resource)
		{
#ifdef _DEBUG
			const D3D12_HEAP_TYPE resourceHeapType{ resource.GetHeapType() };
			assert(resourceHeapType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT && "ERROR: An attempt was made to track the state of a resource created in either a D3D12_HEAP_TYPE_UPLOAD or D3D12_HEAP_TYPE_READBACK heap! (This is unnecessary because resources created in these heaps can never transition out of their initial states. Tracking the resources anyways will likely lead to D3D12 device removal if actual transitions are performed.)");
#endif // _DEBUG
		}

		void GPUResourceUsageAnalyzer::TraverseFrameGraph(const std::span<const GPUExecutionModule> executionModuleSpan)
		{
			for (const auto& executionModule : executionModuleSpan)
				mStateTracker.TrackGPUExecutionModule(executionModule);
		}

		GPUResourceEventManager GPUResourceUsageAnalyzer::ExtractGPUResourceEventManager()
		{
			return mStateTracker.FinalizeStateTracking();
		}
	}
}