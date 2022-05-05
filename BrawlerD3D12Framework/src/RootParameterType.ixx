module;

export module Brawler.D3D12.RootParameterType;

export namespace Brawler
{
	namespace D3D12
	{
		enum class RootParameterType
		{
			CBV,
			SRV,
			UAV,
			ROOT_CONSTANT,
			DESCRIPTOR_TABLE
		};
	}
}