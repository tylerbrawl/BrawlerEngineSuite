module;
#include <functional>
#include <vector>
#include <memory>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.FrameGraph;
import :FrameGraphCallbackCollection;
import Brawler.D3D12.I_RenderModule;
import Brawler.D3D12.TransientGPUResourceManager;
import Brawler.D3D12.FrameGraphExecutionContext;
import Brawler.D3D12.FrameGraphFenceCollection;
import Brawler.D3D12.CommandAllocatorStorage;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.ThreadSafeVector;

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
		private:
			using CallbackType = std::move_only_function<void()>;

		public:
			struct FrameProcessingContext
			{
				std::span<const std::unique_ptr<I_RenderModule>> RenderModuleSpan;
				const Brawler::ThreadSafeVector<std::unique_ptr<CallbackType>>& PersistentCallbackArr;
			};

		public:
			FrameGraph() = default;

			FrameGraph(const FrameGraph& rhs) = delete;
			FrameGraph& operator=(const FrameGraph& rhs) = delete;

			FrameGraph(FrameGraph&& rhs) noexcept = default;
			FrameGraph& operator=(FrameGraph&& rhs) noexcept = default;

			void Initialize();

			void ProcessCurrentFrame(const FrameProcessingContext& context);

			void AddTransientFrameGraphCompletionCallback(CallbackType&& transientCallback);

			Brawler::D3D12CommandAllocator& GetD3D12CommandAllocator(const GPUCommandQueueType queueType);

		private:
			void GenerateFrameGraph(const FrameProcessingContext& context);
			void SubmitFrameGraph();

			void WaitForPreviousFrameGraphExecution() const;
			void ResetFrameGraph();

			std::vector<FrameGraphBuilder> CreateFrameGraphBuilders(const std::span<const std::unique_ptr<I_RenderModule>> renderModuleSpan);
			FrameGraphExecutionContext CompileFrameGraph(const std::span<FrameGraphBuilder> builderSpan);

		private:
			FrameGraphFenceCollection mFenceCollection;
			TransientGPUResourceManager mTransientResourceManager;
			FrameGraphExecutionContext mExecutionContext;
			CommandAllocatorStorage mCmdAllocatorStorage;
			FrameGraphCallbackCollection mCallbackCollection;
		};
	}
}