module;

export module Brawler.D3D12.RootParameterType;

export namespace Brawler
{
	namespace RootParameters
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