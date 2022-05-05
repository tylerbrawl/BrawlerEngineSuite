module;

export module Brawler.D3D12.GPUCommandListType;

export namespace Brawler
{
	namespace D3D12
	{
		enum class GPUCommandListType
		{
			DIRECT,
			COMPUTE,
			COPY
		};
	}
}