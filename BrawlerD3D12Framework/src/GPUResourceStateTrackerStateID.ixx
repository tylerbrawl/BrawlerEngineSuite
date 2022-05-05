module;

export module Brawler.D3D12.GPUResourceStateTrackerStateID;

export namespace Brawler
{
	namespace D3D12
	{
		enum class GPUResourceStateTrackerStateID
		{
			GPU_RESOURCE_SPECIAL_INITIALIZATION,
			BARRIER_TYPE_SELECTOR,
			IMMEDIATE_BARRIER,
			SPLIT_BARRIER,

			COUNT_OR_ERROR
		};
	}
}