module;
#include <vector>
#include <unordered_map>
#include <array>
#include <mutex>

export module Brawler.GPUSceneBufferUpdateSubModule;
import Brawler.I_GPUSceneBufferUpdateSource;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.BufferCopyRegion;

export namespace Brawler
{
	class GPUSceneBufferUpdateSubModule
	{
	private:
		struct GPUSceneBufferUpdateCopyRegions
		{
			const D3D12::BufferCopyRegion* UploadDestRegionPtr;
			D3D12::BufferCopyRegion UploadSrcRegion;
		};

		struct GPUSceneBufferUpdatePassInfo
		{
			std::vector<GPUSceneBufferUpdateCopyRegions> CopyRegionsArr;
		};

	public:
		using BufferUpdateRenderPass_T = D3D12::RenderPass<D3D12::GPUCommandQueueType::COPY, GPUSceneBufferUpdatePassInfo>;

	public:
		GPUSceneBufferUpdateSubModule() = default;

		GPUSceneBufferUpdateSubModule(const GPUSceneBufferUpdateSubModule& rhs) = delete;
		GPUSceneBufferUpdateSubModule& operator=(const GPUSceneBufferUpdateSubModule& rhs) = delete;

		GPUSceneBufferUpdateSubModule(GPUSceneBufferUpdateSubModule&& rhs) noexcept = default;
		GPUSceneBufferUpdateSubModule& operator=(GPUSceneBufferUpdateSubModule&& rhs) noexcept = default;

		void ScheduleGPUSceneBufferUpdateForNextFrame(const I_GPUSceneBufferUpdateSource& bufferUpdateSource);

		bool HasScheduledBufferUpdates() const;

		BufferUpdateRenderPass_T CreateGPUSceneBufferUpdateRenderPass(D3D12::FrameGraphBuilder& builder);

	private:
		std::unordered_set<const I_GPUSceneBufferUpdateSource*> mBufferUpdateSrcPtrSet;
		mutable std::mutex mCritSection;
	};
}