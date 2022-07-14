module;
#include <mutex>
#include <unordered_set>

export module Brawler.GPUSceneUpdateRenderModule;
import Brawler.D3D12.I_RenderModule;
import Brawler.I_GPUSceneBufferUpdateSource;

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

		bool IsRenderModuleEnabled() const override;

	protected:
		void BuildFrameGraph(D3D12::FrameGraphBuilder& builder) override;

	private:
		void UpdateGPUSceneBuffers(D3D12::FrameGraphBuilder& builder);

	private:
		std::unordered_set<const I_GPUSceneBufferUpdateSource*> mBufferUpdateSrcPtrSet;
		mutable std::mutex mCritSection;
	};
}