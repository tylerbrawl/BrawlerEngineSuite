module;
#include <variant>
#include "DxDef.h"

export module Brawler.D3D12.FrameGraphCompilation:FrameGraphInjection;

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
		struct ResourceTransitionBarrier
		{
			I_GPUResource* ResourcePtr;
			D3D12_RESOURCE_STATES StateBefore;
			D3D12_RESOURCE_STATES StateAfter;
		};
	}
}