module;
#include <array>
#include <cassert>
#include "DxDef.h"

module Util.Engine;
import Brawler.D3D12.Renderer;
import Brawler.D3D12.GPUDevice;
import Util.Threading;
import Brawler.ThreadLocalResources;
import Brawler.D3D12.FrameGraphManager;

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
}

namespace Util
{
	namespace Engine
	{
		Brawler::D3D12::GPUDevice& GetGPUDevice()
		{
			thread_local Brawler::D3D12::GPUDevice& gpuDevice{ GetRenderer().GetGPUDevice() };

			return gpuDevice;
		}
		
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

		Brawler::D3D12::GPUVendor GetGPUVendor()
		{
			static const Brawler::D3D12::GPUVendor vendor{ GetGPUDevice().GetGPUVendor() };

			return vendor;
		}

		std::uint64_t GetCurrentFrameNumber()
		{
			const Brawler::ThreadLocalResources& threadLocalResources{ Util::Threading::GetThreadLocalResources() };

			if (threadLocalResources.HasCachedFrameNumber()) [[likely]]
				return threadLocalResources.GetCachedFrameNumber();

			return GetTrueFrameNumber();
		}

		std::uint64_t GetTrueFrameNumber()
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

		Brawler::D3D12CommandAllocator& GetD3D12CommandAllocator(const Brawler::D3D12::GPUCommandQueueType queueType)
		{
			thread_local std::array<Brawler::D3D12CommandAllocator*, MAX_FRAMES_IN_FLIGHT> directCmdAllocatorPtrArr{};
			thread_local std::array<Brawler::D3D12CommandAllocator*, MAX_FRAMES_IN_FLIGHT> computeCmdAllocatorPtrArr{};
			thread_local std::array<Brawler::D3D12CommandAllocator*, MAX_FRAMES_IN_FLIGHT> copyCmdAllocatorPtrArr{};

			static constexpr auto GET_COMMAND_ALLOCATOR_LAMBDA = []<Brawler::D3D12::GPUCommandQueueType QueueType>(std::array<Brawler::D3D12CommandAllocator*, MAX_FRAMES_IN_FLIGHT>& cmdAllocatorPtrArr) -> Brawler::D3D12CommandAllocator&
			{
				const std::size_t currArrIndex = (GetCurrentFrameNumber() % cmdAllocatorPtrArr.size());

				if (cmdAllocatorPtrArr[currArrIndex] == nullptr) [[unlikely]]
					cmdAllocatorPtrArr[currArrIndex] = std::addressof(GetRenderer().GetFrameGraphManager().GetD3D12CommandAllocator(QueueType));

				return *(cmdAllocatorPtrArr[currArrIndex]);
			};

			using CmdQueueType = Brawler::D3D12::GPUCommandQueueType;

			switch (queueType)
			{
			case CmdQueueType::DIRECT:
				return GET_COMMAND_ALLOCATOR_LAMBDA.operator() < CmdQueueType::DIRECT > (directCmdAllocatorPtrArr);

			case CmdQueueType::COMPUTE:
				return GET_COMMAND_ALLOCATOR_LAMBDA.operator() < CmdQueueType::COMPUTE > (computeCmdAllocatorPtrArr);

			case CmdQueueType::COPY:
				return GET_COMMAND_ALLOCATOR_LAMBDA.operator() < CmdQueueType::COPY > (copyCmdAllocatorPtrArr);

			default: [[unlikely]]
			{
				assert(false && "ERROR: An invalid Brawler::D3D12::GPUCommandQueueType value was provided to Util::Engine::GetD3D12CommandAllocator()!");
				std::unreachable();

				return GET_COMMAND_ALLOCATOR_LAMBDA.operator() < CmdQueueType::DIRECT > (directCmdAllocatorPtrArr);
			}
			}
		}

		const Brawler::D3D12::GPUCapabilities& GetGPUCapabilities()
		{
			thread_local const Brawler::D3D12::GPUCapabilities& capabilities{ GetRenderer().GetGPUDevice().GetGPUCapabilities() };

			return capabilities;
		}
	}
}