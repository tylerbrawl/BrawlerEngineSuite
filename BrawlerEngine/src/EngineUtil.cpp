module;
#include <thread>
#include "DxDef.h"

module Util.Engine;
import Brawler.Application;
import Brawler.Renderer;
import Brawler.DisplayAdapter;
import Brawler.D3DVideoBudgetInfo;
import Brawler.CommandListType;
import Brawler.RenderJobManager;

namespace Util
{
	namespace Engine
	{
		// A significant number of the functions in Util::Engine are meant to conveniently access
		// core components of the Brawler Engine, such as the Renderer and DisplayAdapter of the
		// static Application instance. Typically, this involves a large amount of pointer
		// indirections.
		// 
		// However, since we know that these components are initialized at application initialization,
		// remain alive throughout the program's lifetime, and never change their location in memory,
		// we can cache the references as thread_local variables. That way, we do not have to pay for 
		// all of the pointer indirections more than once per thread. (We use thread_local variables
		// so that the functions can easily be called by any thread without needing to use
		// synchronization primitives or atomics for safe access to static variables.)

		Brawler::Renderer& GetRenderer()
		{
			thread_local Brawler::Renderer& renderer{ Brawler::GetApplication().GetRenderer() };

			return renderer;
		}

		Brawler::RenderJobManager& GetRenderJobManager()
		{
			thread_local Brawler::RenderJobManager& jobManager{ GetRenderer().GetRenderJobManager() };

			return jobManager;
		}

		Brawler::DisplayAdapter& GetDisplayAdapter()
		{
			thread_local Brawler::DisplayAdapter& dispAdapter{ GetRenderer().GetDisplayAdapter() };

			return dispAdapter;
		}

		Brawler::DXGIAdapter& GetDXGIAdapter()
		{
			thread_local Brawler::DXGIAdapter& dxgiAdapter{ GetDisplayAdapter().GetAdapter() };

			return dxgiAdapter;
		}

		Brawler::DXGIFactory& GetDXGIFactory()
		{
			thread_local Brawler::DXGIFactory& dxgiFactory{ GetRenderer().GetDXGIFactory() };

			return dxgiFactory;
		}

		Brawler::D3D12Device& GetD3D12Device()
		{
			thread_local Brawler::D3D12Device& d3dDevice{ GetDisplayAdapter().GetD3D12Device() };

			return d3dDevice;
		}

		Brawler::CommandQueue& GetCommandQueue(const Brawler::CommandListType cmdListType)
		{
			return GetDisplayAdapter().GetCommandQueue(cmdListType);
		}

		Brawler::PSOManager& GetPSOManager()
		{
			thread_local Brawler::PSOManager& psoManager{ GetRenderer().GetPSOManager() };

			return psoManager;
		}

		Brawler::D3DVideoBudgetInfo GetVideoBudgetInfo()
		{
			Brawler::DXGIAdapter& dxgiAdapter{ GetDXGIAdapter() };
			Brawler::D3DVideoBudgetInfo budgetInfo{};

			CheckHRESULT(dxgiAdapter.QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &(budgetInfo.GPUBudgetInfo)));
			CheckHRESULT(dxgiAdapter.QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &(budgetInfo.CPUBudgetInfo)));
			
			return budgetInfo;
		}

		std::thread::id GetMasterRenderThreadID()
		{
			return GetRenderJobManager().GetMasterRenderThreadID();
		}

		std::uint64_t GetCurrentUpdateTick()
		{
			return Brawler::GetApplication().GetCurrentUpdateTick();
		}

		std::uint64_t GetCurrentFrameNumber()
		{
			return GetRenderer().GetCurrentFrameNumber();
		}

		std::uint32_t GetDescriptorHandleIncrementSize(const D3D12_DESCRIPTOR_HEAP_TYPE heapType)
		{
			thread_local const std::array<std::uint32_t, 4> HANDLE_INCREMENT_SIZE_ARRAY{
				GetD3D12Device().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
				GetD3D12Device().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
				GetD3D12Device().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV),
				GetD3D12Device().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
			};

			assert(heapType != D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES && "ERROR: An invalid descriptor type was provided to Util::Engine::GetDescriptorHandleIncrementSize()!");
			return HANDLE_INCREMENT_SIZE_ARRAY[heapType];
		}

		bool IsMasterRenderThread()
		{
			return (std::this_thread::get_id() == GetMasterRenderThreadID());
		}
	}
}