module;
#include <memory>

export module Brawler.GPUSceneUpdateRenderModule;
import Brawler.D3D12.I_RenderModule;
import Brawler.GPUSceneBufferUpdateSubModule;
import Brawler.GenericPreFrameUpdateSubModule;
import Brawler.GenericPreFrameBufferUpdate;
import Brawler.GenericPreFrameTextureUpdate;
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

		void ScheduleGenericBufferUpdateForNextFrame(GenericPreFrameBufferUpdate&& preFrameUpdate);
		void ScheduleGenericTextureUpdateForNextFrame(GenericPreFrameTextureUpdate&& preFrameUpdate);

		bool IsRenderModuleEnabled() const override;

	protected:
		void BuildFrameGraph(D3D12::FrameGraphBuilder& builder) override;

	private:
		GPUSceneBufferUpdateSubModule mBufferUpdateSubModule;
		GenericPreFrameUpdateSubModule mPreFrameUpdateSubModule;
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