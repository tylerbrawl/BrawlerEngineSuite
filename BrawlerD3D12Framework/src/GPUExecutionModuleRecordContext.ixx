module;

export module Brawler.D3D12.GPUExecutionModuleRecordContext;

export namespace Brawler
{
	namespace D3D12
	{
		class TransientGPUResourceAliasTracker;
		class GPUResourceEventManager;
		class GPUCommandContextSubmissionPoint;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		struct GPUExecutionModuleRecordContext
		{
			const TransientGPUResourceAliasTracker* AliasTracker;
			GPUResourceEventManager* EventManager;
			GPUCommandContextSubmissionPoint* SubmitPoint;
		};
	}
}