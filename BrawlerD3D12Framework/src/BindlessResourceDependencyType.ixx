module;

export module Brawler.D3D12.BindlessResourceDependencyType;

export namespace Brawler
{
	namespace D3D12
	{
		enum class BindlessResourceDependencyType
		{
			PIXEL_SHADER_RESOURCE,
			NON_PIXEL_SHADER_RESOURCE,

			COUNT_OR_ERROR
		};
	}
}