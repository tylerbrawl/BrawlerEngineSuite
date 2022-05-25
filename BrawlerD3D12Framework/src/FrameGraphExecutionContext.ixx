module;
#include <span>
#include <vector>

export module Brawler.D3D12.FrameGraphExecutionContext;
import Brawler.D3D12.TransientGPUResourceAliasTracker;
import Brawler.D3D12.GPUExecutionModule;
import Brawler.D3D12.GPUResourceEventManager;
import Brawler.D3D12.FrameGraphFenceCollection;

export namespace Brawler
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
		class FrameGraphExecutionContext
		{
		public:
			FrameGraphExecutionContext() = default;

			FrameGraphExecutionContext(const FrameGraphExecutionContext& rhs) = delete;
			FrameGraphExecutionContext& operator=(const FrameGraphExecutionContext& rhs) = delete;

			FrameGraphExecutionContext(FrameGraphExecutionContext&& rhs) noexcept = default;
			FrameGraphExecutionContext& operator=(FrameGraphExecutionContext&& rhs) noexcept = default;

			void CompileFrameGraph(const std::span<FrameGraphBuilder> builderSpan, TransientGPUResourceAliasTracker&& aliasTracker);
			void SubmitFrameGraph(FrameGraphFenceCollection& fenceCollection);

		private:
			void CreateGPUExecutionModules(const std::span<FrameGraphBuilder> builderSpan);
			void PerformGPUResourceAnalysis();

		private:
			std::vector<GPUExecutionModule> mExecutionModuleArr;
			TransientGPUResourceAliasTracker mAliasTracker;
			GPUResourceEventManager mEventManager;
		};
	}
}