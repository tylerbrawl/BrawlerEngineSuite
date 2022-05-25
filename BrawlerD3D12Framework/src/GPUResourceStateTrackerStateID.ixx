module;

export module Brawler.D3D12.GPUResourceStateTrackerStateID;

export namespace Brawler
{
	namespace D3D12
	{
		enum class GPUResourceStateTrackerStateID
		{
			EXPLICIT_BARRIER,
			IMPLICIT_BARRIER,

			COUNT_OR_ERROR
		};
	}
}