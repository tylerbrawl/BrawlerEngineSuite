module;

export module Brawler.D3D12.ResourceStateZoneOptimizerStateID;

export namespace Brawler
{
	namespace D3D12
	{
		enum class ResourceStateZoneOptimizerStateID
		{
			READ_RESOURCE_STATE_ZONE,
			IGNORE_RESOURCE_STATE_ZONE,

			COUNT_OR_ERROR
		};
	}
}