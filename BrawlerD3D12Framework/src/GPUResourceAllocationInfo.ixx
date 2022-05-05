module;
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceAllocationInfo;

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		struct GPUResourceAllocationInfo
		{
			std::span<I_GPUResource* const> AliasedResources;
			D3D12_RESOURCE_ALLOCATION_INFO AllocationInfo;
		};
	}
}