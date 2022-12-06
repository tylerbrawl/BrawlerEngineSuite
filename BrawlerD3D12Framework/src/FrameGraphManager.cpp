module;
#include <functional>
#include <span>
#include <memory>
#include <vector>
#include <array>
#include <cassert>
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
			const FrameGraph::FrameProcessingContext frameProcessingContext{
				.RenderModuleSpan{ mRenderModuleArr },
				.PersistentCallbackArr{ mPersistentCallbackPtrArr }
			};

			currFrameGraph.ProcessCurrentFrame(frameProcessingContext);
		}

		void FrameGraphManager::AddPersistentFrameGraphCompletionCallback(CallbackType&& persistentCallback)
		{
			// Invoking an empty std::move_only_function instance results in undefined behavior.
			assert(persistentCallback && "ERROR: An attempt was made to specify an empty std::move_only_function instance in a call to FrameGraphManager::AddPersistentFrameGraphCompletionCallback()!");

			mPersistentCallbackPtrArr.PushBack(std::make_unique<CallbackType>(std::move(persistentCallback)));
		}

		void FrameGraphManager::AddTransientFrameGraphCompletionCallback(CallbackType&& transientCallback)
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