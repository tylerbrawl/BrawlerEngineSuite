module;
#include <utility>
#include <type_traits>

export module Brawler.GPUSceneBufferUpdater;
import Brawler.D3D12.BufferCopyRegion;
import Brawler.Application;
import Brawler.D3D12.Renderer;
import Brawler.GPUSceneUpdateRenderModule;
import Brawler.GPUSceneBufferMap;
import Brawler.GPUSceneBufferUpdateOperation;
import Brawler.GPUSceneBufferID;

export namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	class GPUSceneBufferUpdater
	{
	public:
		using ElementType = GPUSceneBufferElementType<BufferID>;

	public:
		GPUSceneBufferUpdater() = default;
		explicit GPUSceneBufferUpdater(D3D12::BufferCopyRegion&& gpuSceneBufferCopyDest);

		GPUSceneBufferUpdater(const GPUSceneBufferUpdater& rhs) = delete;
		GPUSceneBufferUpdater& operator=(const GPUSceneBufferUpdater& rhs) = delete;

		GPUSceneBufferUpdater(GPUSceneBufferUpdater&& rhs) noexcept = default;
		GPUSceneBufferUpdater& operator=(GPUSceneBufferUpdater&& rhs) noexcept = default;

		template <typename U>
			requires std::is_same_v<std::decay_t<GPUSceneBufferElementType<BufferID>>, std::decay_t<U>>
		void UpdateGPUSceneData(U&& newValue) const;

	private:
		D3D12::BufferCopyRegion mCopyDestRegion;
	};
}

// -------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	GPUSceneBufferUpdater<BufferID>::GPUSceneBufferUpdater(D3D12::BufferCopyRegion&& gpuSceneBufferCopyDest) :
		mCopyDestRegion(std::move(gpuSceneBufferCopyDest))
	{}

	template <GPUSceneBufferID BufferID>
	template <typename U>
		requires std::is_same_v<std::decay_t<GPUSceneBufferElementType<BufferID>>, std::decay_t<U>>
	void GPUSceneBufferUpdater<BufferID>::UpdateGPUSceneData(U&& newValue) const
	{
		// Create a GPUSceneBufferUpdateOperation for this update.
		GPUSceneBufferUpdateOperation<BufferID> updateOperation{ mCopyDestRegion };
		updateOperation.SetUpdateSourceData(std::forward<U>(newValue));

		// Notify the GPUSceneUpdateRenderModule that we have new data which needs to be sent to
		// the GPU.
		Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGPUSceneBufferUpdateForNextFrame(std::move(updateOperation));
	}
}