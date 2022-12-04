module;
#include <functional>
#include <vector>
#include <memory>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.FrameGraph;
import :FrameGraphCallbackCollection;
import Brawler.D3D12.I_RenderModule;
import Brawler.D3D12.FrameGraphBlackboard;
import Brawler.D3D12.TransientGPUResourceManager;
import Brawler.D3D12.FrameGraphExecutionContext;
import Brawler.D3D12.FrameGraphFenceCollection;
import Brawler.D3D12.CommandAllocatorStorage;
import Brawler.D3D12.GPUCommandQueueType;

namespace Brawler
{
	namespace D3D12
	{
		class FrameGraphBuilder;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class FrameGraph
		{
		public:
			FrameGraph() = default;

			FrameGraph(const FrameGraph& rhs) = delete;
			FrameGraph& operator=(const FrameGraph& rhs) = delete;

			FrameGraph(FrameGraph&& rhs) noexcept = default;
			FrameGraph& operator=(FrameGraph&& rhs) noexcept = default;

			void Initialize();

			void ProcessCurrentFrame(const std::span<const std::unique_ptr<I_RenderModule>> renderModuleSpan);

			void AddPersistentFrameGraphCompletionCallback(std::move_only_function<void()>&& persistentCallback);
			void AddTransientFrameGraphCompletionCallback(std::move_only_function<void()>&& transientCallback);

			FrameGraphBlackboard& GetBlackboard();
			const FrameGraphBlackboard& GetBlackboard() const;

			Brawler::D3D12CommandAllocator& GetD3D12CommandAllocator(const GPUCommandQueueType queueType);

		private:
			void GenerateFrameGraph(const std::span<const std::unique_ptr<I_RenderModule>> renderModuleSpan);
			void SubmitFrameGraph();

			void WaitForPreviousFrameGraphExecution() const;
			void ResetFrameGraph();

			std::vector<FrameGraphBuilder> CreateFrameGraphBuilders(const std::span<const std::unique_ptr<I_RenderModule>> renderModuleSpan);
			FrameGraphExecutionContext CompileFrameGraph(const std::span<FrameGraphBuilder> builderSpan);

		private:
			FrameGraphFenceCollection mFenceCollection;
			TransientGPUResourceManager mTransientResourceManager;
			FrameGraphBlackboard mBlackboard;
			FrameGraphExecutionContext mExecutionContext;
			CommandAllocatorStorage mCmdAllocatorStorage;
			FrameGraphCallbackCollection mCallbackCollection;
		};
	}
}