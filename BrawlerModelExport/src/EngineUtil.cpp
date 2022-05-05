module;
#include "DxDef.h"

module Util.Engine;
import Brawler.Application;
import Brawler.D3D12.Renderer;
import Brawler.D3D12.GPUDevice;

namespace
{
	Brawler::D3D12::Renderer& GetRenderer()
	{
		thread_local Brawler::D3D12::Renderer& renderer{ Brawler::GetApplication().GetRenderer() };

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
		Brawler::D3D12::RootSignatureDatabase& GetRootSignatureDatabase()
		{
			thread_local Brawler::D3D12::RootSignatureDatabase& rsDatabase{ GetRenderer().GetRootSignatureDatabase() };

			return rsDatabase;
		}

		Brawler::D3D12::PSODatabase& GetPSODatabase()
		{
			thread_local Brawler::D3D12::PSODatabase& psoDatabase{ GetRenderer().GetPSODatabase() };

			return psoDatabase;
		}

		std::uint64_t GetCurrentFrameNumber()
		{
			return GetRenderer().GetCurrentFrameNumber();
		}

		Brawler::D3D12::GPUCommandManager& GetGPUCommandManager()
		{
			thread_local Brawler::D3D12::GPUCommandManager& cmdManager{ GetRenderer().GetGPUCommandManager() };

			return cmdManager;
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

		D3D12MA::Allocator& GetD3D12Allocator()
		{
			thread_local D3D12MA::Allocator& allocator{ GetRenderer().GetD3D12Allocator() };

			return allocator;
		}
	}
}