module;
#include "DxDef.h"

export module Brawler.D3D12.FrameGraphResourceDependency;
export import Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		struct FrameGraphResourceDependency
		{
			I_GPUResource* ResourcePtr;
			D3D12_RESOURCE_STATES RequiredState;
		};
	}
}