module;
#include "DxDef.h"

export module Brawler.D3D12.BufferResourceInitializationInfo;

export namespace Brawler
{
	namespace D3D12
	{
		struct BufferResourceInitializationInfo
		{
			std::size_t SizeInBytes;
			D3D12_RESOURCE_STATES InitialResourceState;
			D3D12_HEAP_TYPE HeapType;
		};
	}
}