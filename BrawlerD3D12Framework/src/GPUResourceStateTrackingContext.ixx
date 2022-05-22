module;

export module Brawler.D3D12.GPUResourceStateTrackingContext;
import Brawler.D3D12.GPUExecutionModule;
import Brawler.D3D12.GPUResourceEventManager;
import Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceStateBarrierMerger;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		struct GPUResourceStateTrackingContext
		{
			I_GPUResource& GPUResource;
			GPUResourceEventManager& EventManager;
			GPUResourceStateBarrierMerger& BarrierMerger;
			const GPUExecutionModule& ExecutionModule;
		};
	}
}