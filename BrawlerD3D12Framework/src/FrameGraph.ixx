module;
#include <vector>
#include <memory>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.FrameGraph;
import Brawler.D3D12.I_RenderModule;
import Brawler.D3D12.FrameGraphBlackboard;
import Brawler.D3D12.TransientGPUResourceManager;
import Brawler.D3D12.FrameGraphExecutionContext;
import Brawler.D3D12.FrameGraphFenceCollection;

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

			void GenerateFrameGraph();
			void SubmitFrameGraph();

			FrameGraphBlackboard& GetBlackboard();
			const FrameGraphBlackboard& GetBlackboard() const;

		private:
			void CreateRenderModules();
			void WaitForPreviousFrameGraphExecution() const;
			void ResetFrameGraph();

			std::vector<FrameGraphBuilder> CreateFrameGraphBuilders();
			FrameGraphExecutionContext CompileFrameGraph(const std::span<FrameGraphBuilder> builderSpan);

		private:
			FrameGraphFenceCollection mFenceCollection;
			TransientGPUResourceManager mTransientResourceManager;
			std::vector<std::unique_ptr<I_RenderModule>> mRenderModuleArr;
			FrameGraphBlackboard mBlackboard;
			FrameGraphExecutionContext mExecutionContext;
		};
	}
}