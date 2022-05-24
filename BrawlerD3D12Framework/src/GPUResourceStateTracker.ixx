module;
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceStateTracker;
import Brawler.D3D12.GPUResourceEventCollection;
import Brawler.D3D12.GPUResourceStateBarrierMerger;
import Brawler.D3D12.GPUCommandQueueType;
export import Brawler.D3D12.GPUExecutionModule;
import Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceStateTracker
		{
		public:
			GPUResourceStateTracker() = delete;
			explicit GPUResourceStateTracker(I_GPUResource& trackedResource);

			GPUResourceStateTracker(const GPUResourceStateTracker& rhs) = delete;
			GPUResourceStateTracker& operator=(const GPUResourceStateTracker& rhs) = delete;

			GPUResourceStateTracker(GPUResourceStateTracker&& rhs) noexcept = default;
			GPUResourceStateTracker& operator=(GPUResourceStateTracker&& rhs) noexcept = default;

			void TrackGPUExecutionModule(const GPUExecutionModule& executionModule);
			GPUResourceEventCollection FinalizeStateTracking();

		private:
			void CheckForResourceStateDecay(const GPUExecutionModule& executionModule);

		private:
			GPUResourceStateBarrierMerger mBarrierMerger;
			bool mResourceAlwaysDecays;
		};
	}
}