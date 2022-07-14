module;
#include <span>

export module Brawler.GPUSceneBufferUpdater;
import Brawler.I_GPUSceneBufferUpdateSource;
import Brawler.D3D12.BufferCopyRegion;
import Brawler.Application;
import Brawler.Renderer;
import Brawler.GPUSceneUpdateRenderModule;
import Brawler.GPUSceneBufferMap;
import Brawler.GPUSceneBufferID;
import Brawler.D3D12.BufferResource;
import Brawler.GPUSceneManager;

export namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	class GPUSceneBufferUpdater final : public I_GPUSceneBufferUpdateSource
	{
	public:
		using ElementType = GPUSceneBufferElementType<BufferID>;

	public:
		GPUSceneBufferUpdater() = default;
		explicit GPUSceneBufferUpdater(D3D12::BufferCopyRegion&& gpuSceneBufferCopyDest) requires std::is_default_constructible_v<ElementType>;

		GPUSceneBufferUpdater(const GPUSceneBufferUpdater& rhs) = delete;
		GPUSceneBufferUpdater& operator=(const GPUSceneBufferUpdater& rhs) = delete;

		GPUSceneBufferUpdater(GPUSceneBufferUpdater&& rhs) noexcept = default;
		GPUSceneBufferUpdater& operator=(GPUSceneBufferUpdater&& rhs) noexcept = default;

		template <typename U>
			requires std::is_same_v<std::decay_t<ElementType>, std::decay_t<U>>
		void UpdateGPUSceneData(U&& newValue);

		std::span<const std::byte> GetGPUSceneUploadData() const override;
		D3D12::BufferResource& GetGPUSceneBufferResource() const override;

	private:
		ElementType mDataToUpload;
	};
}

// -------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	GPUSceneBufferUpdater<BufferID>::GPUSceneBufferUpdater(D3D12::BufferCopyRegion&& gpuSceneBufferCopyDest) requires std::is_default_constructible_v<ElementType> :
		I_GPUSceneBufferUpdateSource(std::move(gpuSceneBufferCopyDest)),
		mDataToUpload()
	{}

	template <GPUSceneBufferID BufferID>
	template <typename U>
		requires std::is_same_v<std::decay_t<GPUSceneBufferElementType<BufferID>>, std::decay_t<U>>
	void GPUSceneBufferUpdater<BufferID>::UpdateGPUSceneData(U&& newValue)
	{
		mDataToUpload = std::forward<U>(newValue);

		// Notify the GPUSceneUpdateRenderModule that we have new data which needs to be sent to
		// the GPU.
		Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGPUSceneBufferUpdateForNextFrame(*this);
	}

	template <GPUSceneBufferID BufferID>
	std::span<const std::byte> GPUSceneBufferUpdater<BufferID>::GetGPUSceneUploadData() const
	{
		const std::span<const ElementType> dataSpan{ 1, &mDataToUpload };
		return std::as_bytes(dataSpan);
	}

	template <GPUSceneBufferID BufferID>
	D3D12::BufferResource& GPUSceneBufferUpdater<BufferID>::GetGPUSceneBufferResource() const
	{
		return GPUSceneManager::GetInstance().GetGPUSceneBufferResource<BufferID>();
	}
}