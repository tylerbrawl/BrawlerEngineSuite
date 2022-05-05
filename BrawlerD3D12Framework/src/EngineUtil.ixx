module;
#include "DxDef.h"

export module Util.Engine;
import Brawler.D3D12.BindlessSRVAllocation;

export namespace Brawler
{
	namespace D3D12
	{
		class RootSignatureDatabase;
		class PSODatabase;
		class GPUResourceDescriptorHeap;
		struct GPUCapabilities;
		class GPUCommandManager;
		class PersistentGPUResourceManager;
		class GPUResidencyManager;
	}
}

export namespace Util
{
	namespace Engine
	{
		constexpr std::size_t MAX_FRAMES_IN_FLIGHT = 2;
		
		Brawler::D3D12::RootSignatureDatabase& GetRootSignatureDatabase();
		Brawler::D3D12::PSODatabase& GetPSODatabase();

		Brawler::D3D12::GPUCommandManager& GetGPUCommandManager();

		Brawler::D3D12::PersistentGPUResourceManager& GetPersistentGPUResourceManager();

		Brawler::D3D12::GPUResidencyManager& GetGPUResidencyManager();

		std::uint64_t GetCurrentFrameNumber();

		Brawler::D3D12::GPUResourceDescriptorHeap& GetGPUResourceDescriptorHeap();
		std::uint32_t GetDescriptorHandleIncrementSize(const D3D12_DESCRIPTOR_HEAP_TYPE descriptorType);
		
		Brawler::D3D12Device& GetD3D12Device();
		Brawler::DXGIAdapter& GetDXGIAdapter();

		const Brawler::D3D12::GPUCapabilities& GetGPUCapabilities();
	}
}