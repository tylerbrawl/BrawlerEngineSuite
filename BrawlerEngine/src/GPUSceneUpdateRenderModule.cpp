module;
#include <memory>

module Brawler.GPUSceneUpdateRenderModule;

namespace Brawler
{
	void GPUSceneUpdateRenderModule::CommitGlobalTextureChanges(std::unique_ptr<GlobalTextureUploadBuffer>&& preparedBufferPtr)
	{
		mVTManagementSubModule.CommitGlobalTextureChanges(std::move(preparedBufferPtr));
	}

	bool GPUSceneUpdateRenderModule::IsRenderModuleEnabled() const
	{
		// Don't bother trying to create RenderPass instances for GPU scene updates if there are
		// no updates to perform this frame.
		
		return (mBufferUpdateSubModule.HasScheduledBufferUpdates() || mVTManagementSubModule.HasCommittedGlobalTextureChanges());
	}

	void GPUSceneUpdateRenderModule::BuildFrameGraph(D3D12::FrameGraphBuilder& builder)
	{
		
	}
}