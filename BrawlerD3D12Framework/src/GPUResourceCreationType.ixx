module;

export module Brawler.D3D12.GPUResourceCreationType;

export namespace Brawler
{
	namespace D3D12
	{
		enum class GPUResourceCreationType
		{
			COMMITTED,
			PLACED,

			COUNT_OR_ERROR
		};
	}
}