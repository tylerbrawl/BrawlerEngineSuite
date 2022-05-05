module;
#include "DxDef.h"

export module Util.Engine;

export namespace Brawler
{
	namespace D3D12
	{
		class RootSignatureDatabase;
		class PSODatabase;
		class GPUResourceDescriptorHeap;
		class GPUCommandManager;
	}
}

export namespace Util
{
	namespace Engine
	{
		Brawler::D3D12::RootSignatureDatabase& GetRootSignatureDatabase();
		Brawler::D3D12::PSODatabase& GetPSODatabase();

		std::uint64_t GetCurrentFrameNumber();

		Brawler::D3D12::GPUCommandManager& GetGPUCommandManager();

		Brawler::D3D12::GPUResourceDescriptorHeap& GetGPUResourceDescriptorHeap();
		std::uint32_t GetDescriptorHandleIncrementSize(const D3D12_DESCRIPTOR_HEAP_TYPE descriptorType);
		
		Brawler::D3D12Device& GetD3D12Device();
		D3D12MA::Allocator& GetD3D12Allocator();
	}
}