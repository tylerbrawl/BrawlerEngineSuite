module;
#include <functional>
#include <span>
#include <memory>
#include <vector>
#include <array>
#include "DxDef.h"

module Brawler.D3D12.FrameGraphManager;

namespace Brawler
{
	namespace D3D12
	{
		void FrameGraphManager::Initialize()
		{
			for (auto& frameGraph : mFrameGraphArr)
				frameGraph.Initialize();
		}

		void FrameGraphManager::ProcessCurrentFrame()
		{
			FrameGraph& currFrameGraph{ GetCurrentFrameGraph() };
			currFrameGraph.ProcessCurrentFrame(std::span<const std::unique_ptr<I_RenderModule>>{ mRenderModuleArr });
		}

		void FrameGraphManager::AddPersistentFrameGraphCompletionCallback(std::move_only_function<void()>&& persistentCallback)
		{
			GetCurrentFrameGraph().AddPersistentFrameGraphCompletionCallback(std::move(persistentCallback));
		}

		void FrameGraphManager::AddTransientFrameGraphCompletionCallback(std::move_only_function<void()>&& transientCallback)
		{
			GetCurrentFrameGraph().AddTransientFrameGraphCompletionCallback(std::move(transientCallback));
		}

		Brawler::D3D12CommandAllocator& FrameGraphManager::GetD3D12CommandAllocator(const GPUCommandQueueType queueType)
		{
			return GetCurrentFrameGraph().GetD3D12CommandAllocator(queueType);
		}

		FrameGraph& FrameGraphManager::GetCurrentFrameGraph()
		{
			return mFrameGraphArr[Util::Engine::GetCurrentFrameNumber() % Util::Engine::MAX_FRAMES_IN_FLIGHT];
		}

		const FrameGraph& FrameGraphManager::GetCurrentFrameGraph() const
		{
			return mFrameGraphArr[Util::Engine::GetCurrentFrameNumber() % Util::Engine::MAX_FRAMES_IN_FLIGHT];
		}
	}
}