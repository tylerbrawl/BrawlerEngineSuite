module;
#include <memory>

export module Brawler.GPUSceneUpdateRenderModule;
import Brawler.D3D12.I_RenderModule;
import Brawler.GPUSceneBufferUpdateSubModule;
import Brawler.GlobalTextureUploadBuffer;
import Brawler.VirtualTextureManagementSubModule;
import Brawler.GPUSceneBufferID;
import Brawler.GPUSceneBufferUpdateOperation;

export namespace Brawler
{
	class GPUSceneUpdateRenderModule final : public D3D12::I_RenderModule
	{
	public:
		GPUSceneUpdateRenderModule() = default;

		GPUSceneUpdateRenderModule(const GPUSceneUpdateRenderModule& rhs) = delete;
		GPUSceneUpdateRenderModule& operator=(const GPUSceneUpdateRenderModule& rhs) = delete;

		GPUSceneUpdateRenderModule(GPUSceneUpdateRenderModule&& rhs) noexcept = default;
		GPUSceneUpdateRenderModule& operator=(GPUSceneUpdateRenderModule&& rhs) noexcept = default;

		template <GPUSceneBufferID BufferID>
		void ScheduleGPUSceneBufferUpdateForNextFrame(GPUSceneBufferUpdateOperation<BufferID>&& updateOperation);

		void CommitGlobalTextureChanges(std::unique_ptr<GlobalTextureUploadBuffer>&& preparedBufferPtr);

		bool IsRenderModuleEnabled() const override;

	protected:
		void BuildFrameGraph(D3D12::FrameGraphBuilder& builder) override;

	private:
		GPUSceneBufferUpdateSubModule mBufferUpdateSubModule;
		VirtualTextureManagementSubModule mVTManagementSubModule;
	};
}

// ----------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	void GPUSceneUpdateRenderModule::ScheduleGPUSceneBufferUpdateForNextFrame(GPUSceneBufferUpdateOperation<BufferID>&& updateOperation)
	{
		mBufferUpdateSubModule.ScheduleGPUSceneBufferUpdateForNextFrame(std::move(updateOperation));
	}
}