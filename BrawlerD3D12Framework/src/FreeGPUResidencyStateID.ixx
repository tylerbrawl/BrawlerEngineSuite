module;

export module Brawler.D3D12.FreeGPUResidencyStateID;

export namespace Brawler
{
	namespace D3D12
	{
		enum class FreeGPUResidencyStateID
		{
			EVICT_PAGEABLE_GPU_OBJECT,
			DELETE_PAGEABLE_GPU_OBJECT,

			COUNT_OR_ERROR
		};
	}
}