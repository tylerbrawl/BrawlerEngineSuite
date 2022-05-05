module;

export module Brawler.D3D12.GPUCommandQueueType;

export namespace Brawler
{
	namespace D3D12
	{
		enum class GPUCommandQueueType
		{
			DIRECT,
			COMPUTE,
			COPY,

			COUNT_OR_ERROR
		};
	}
}