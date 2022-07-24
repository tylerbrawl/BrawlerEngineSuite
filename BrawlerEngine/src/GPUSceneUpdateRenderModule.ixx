module;
#include <memory>

export module Brawler.GPUSceneUpdateRenderModule;
import Brawler.D3D12.I_RenderModule;
import Brawler.I_GPUSceneBufferUpdateSource;
import Brawler.GPUSceneBufferUpdateSubModule;
import Brawler.GlobalTextureUpdateBuffer;
import Brawler.VirtualTextureManagementSubModule;

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

		void ScheduleGPUSceneBufferUpdateForNextFrame(const I_GPUSceneBufferUpdateSource& bufferUpdateSource);
		void CommitGlobalTextureChanges(std::unique_ptr<GlobalTextureUploadBuffer>&& preparedBufferPtr);

		bool IsRenderModuleEnabled() const override;

	protected:
		void BuildFrameGraph(D3D12::FrameGraphBuilder& builder) override;

	private:
		GPUSceneBufferUpdateSubModule mBufferUpdateSubModule;
		VirtualTextureManagementSubModule mVTManagementSubModule;
	};
}