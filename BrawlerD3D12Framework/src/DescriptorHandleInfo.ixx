module;
#include "DxDef.h"

export module Brawler.D3D12.DescriptorHandleInfo;

export namespace Brawler
{
	namespace D3D12
	{
		struct DescriptorHandleInfo
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE HCPUDescriptor;
			CD3DX12_GPU_DESCRIPTOR_HANDLE HGPUDescriptor;
		};
	}
}