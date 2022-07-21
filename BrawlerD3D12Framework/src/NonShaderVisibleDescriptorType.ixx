module;

export module Brawler.D3D12.NonShaderVisibleDescriptorType;

export namespace Brawler
{
	namespace D3D12
	{
		enum class NonShaderVisibleDescriptorType
		{
			RTV,
			DSV,

			COUNT_OR_ERROR
		};
	}
}