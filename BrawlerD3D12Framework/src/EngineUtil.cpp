module;
#include "DxDef.h"

module Util.Engine;
import Brawler.D3D12.Renderer;
import Brawler.D3D12.GPUDevice;

namespace Brawler
{
	extern D3D12::Renderer& GetRenderer();
}

namespace
{
	Brawler::D3D12::Renderer& GetRenderer()
	{
		thread_local Brawler::D3D12::Renderer& renderer{ Brawler::GetRenderer() };

		return renderer;
	}

	Brawler::D3D12::GPUDevice& GetGPUDevice()
	{
		thread_local Brawler::D3D12::GPUDevice& gpuDevice{ GetRenderer().GetGPUDevice() };

		return gpuDevice;
	}
}

namespace Util
{
	namespace Engine
	{
		Brawler::D3D12::GPUCommandManager& GetGPUCommandManager()
		{
			thread_local Brawler::D3D12::GPUCommandManager& cmdManager{ GetRenderer().GetGPUCommandManager() };

			return cmdManager;
		}

		Brawler::D3D12::PersistentGPUResourceManager& GetPersistentGPUResourceManager()
		{
			thread_local Brawler::D3D12::PersistentGPUResourceManager& resourceManager{ GetRenderer().GetPersistentGPUResourceManager() };

			return resourceManager;
		}

		Brawler::D3D12::GPUResidencyManager& GetGPUResidencyManager()
		{
			thread_local Brawler::D3D12::GPUResidencyManager& residencyManager{ GetGPUDevice().GetGPUResidencyManager() };

			return residencyManager;
		}

		std::uint64_t GetCurrentFrameNumber()
		{
			return GetRenderer().GetCurrentFrameNumber();
		}

		Brawler::D3D12::GPUResourceDescriptorHeap& GetGPUResourceDescriptorHeap()
		{
			thread_local Brawler::D3D12::GPUResourceDescriptorHeap& descriptorHeap{ GetGPUDevice().GetGPUResourceDescriptorHeap() };

			return descriptorHeap;
		}

		std::uint32_t GetDescriptorHandleIncrementSize(const D3D12_DESCRIPTOR_HEAP_TYPE descriptorType)
		{
			return GetGPUDevice().GetDescriptorHandleIncrementSize(descriptorType);
		}
		
		Brawler::D3D12Device& GetD3D12Device()
		{
			thread_local Brawler::D3D12Device& d3dDevice{ GetRenderer().GetGPUDevice().GetD3D12Device() };

			return d3dDevice;
		}

		Brawler::DXGIAdapter& GetDXGIAdapter()
		{
			thread_local Brawler::DXGIAdapter& dxgiAdapter{ GetRenderer().GetGPUDevice().GetDXGIAdapter() };

			return dxgiAdapter;
		}

		const Brawler::D3D12::GPUCapabilities& GetGPUCapabilities()
		{
			thread_local const Brawler::D3D12::GPUCapabilities& capabilities{ GetRenderer().GetGPUDevice().GetGPUCapabilities() };

			return capabilities;
		}
	}
}