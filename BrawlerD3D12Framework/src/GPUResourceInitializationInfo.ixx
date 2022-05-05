module;
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceInitializationInfo;

export namespace Brawler
{
	namespace D3D12
	{
		struct GPUResourceInitializationInfo
		{
			Brawler::D3D12_RESOURCE_DESC ResourceDesc;
			D3D12_RESOURCE_STATES InitialResourceState;
			D3D12_HEAP_TYPE HeapType;
		};
	}
}