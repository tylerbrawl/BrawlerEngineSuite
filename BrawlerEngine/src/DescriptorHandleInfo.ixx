module;
#include "DxDef.h"

export module Brawler.DescriptorHandleInfo;

export namespace Brawler
{
	struct DescriptorHandleInfo
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE GPUHandle;
	};
}