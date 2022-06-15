module;
#include <cassert>

module Brawler.D3D12.I_RenderModule;
import Brawler.D3D12.FrameGraphBuilder;
import Brawler.D3D12.I_GPUResource;

namespace Brawler
{
	namespace D3D12
	{
		bool I_RenderModule::IsRenderModuleEnabled() const
		{
			return true;
		}

		void I_RenderModule::CreateFrameGraphBuilder(FrameGraphBuilder& builder)
		{
			// Allow derived instances to construct the FrameGraphBuilder by adding
			// RenderPasses, creating transient resources, etc.
			BuildFrameGraph(builder);

			// Finalize the FrameGraphBuilder. We do this here to reduce the amount
			// of work required when adding its components to the FrameGraph.
			FinalizeFrameGraphBuilder(builder);
		}

		void I_RenderModule::FinalizeFrameGraphBuilder(FrameGraphBuilder& builder) const
		{
#ifdef _DEBUG
			for (const auto& bundle : builder.GetRenderPassBundleSpan())
				assert(bundle.GetUsedQueues() != GPUCommandQueueType::COMPUTE && "ERROR: A RenderPassBundle containing only COMPUTE RenderPasses was detected! (The COMPUTE queue is meant for asynchronous compute. Compute shaders which are executed independently should be recorded with a DIRECT RenderPass. This assertion can be disabled in I_RenderModule.cpp if you really know what you are doing.)");
#endif // _DEBUG
			
			builder.CreateSyncPoints();
			builder.UpdateResourceUsageForCurrentFrame();
		}
	}
}