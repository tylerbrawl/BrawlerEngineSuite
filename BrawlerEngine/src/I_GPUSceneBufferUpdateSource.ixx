module;
#include <span>

export module Brawler.I_GPUSceneBufferUpdateSource;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.BufferCopyRegion;

export namespace Brawler
{
	class I_GPUSceneBufferUpdateSource
	{
	protected:
		I_GPUSceneBufferUpdateSource() = default;
		explicit I_GPUSceneBufferUpdateSource(D3D12::BufferCopyRegion&& gpuSceneBufferCopyDest);

	public:
		virtual ~I_GPUSceneBufferUpdateSource() = default;

		I_GPUSceneBufferUpdateSource(const I_GPUSceneBufferUpdateSource& rhs) = delete;
		I_GPUSceneBufferUpdateSource& operator=(const I_GPUSceneBufferUpdateSource& rhs) = delete;

		I_GPUSceneBufferUpdateSource(I_GPUSceneBufferUpdateSource&& rhs) noexcept = default;
		I_GPUSceneBufferUpdateSource& operator=(I_GPUSceneBufferUpdateSource&& rhs) noexcept = default;

		virtual std::span<const std::byte> GetGPUSceneUploadData() const = 0;
		virtual D3D12::BufferResource& GetGPUSceneBufferResource() const = 0;

		const D3D12::BufferCopyRegion& GetGPUSceneBufferCopyDestination() const;

	private:
		D3D12::BufferCopyRegion mCopyDestRegion;
	};
}