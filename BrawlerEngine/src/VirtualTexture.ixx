module;
#include <memory>

export module Brawler.VirtualTexture;
import Brawler.FilePathHash;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneTypes;
import Brawler.GPUSceneBufferUpdater;
import Brawler.GPUSceneBufferID;

export namespace Brawler
{
	class VirtualTexture
	{
	public:
		explicit VirtualTexture(const FilePathHash bvtxFileHash);

		VirtualTexture(const VirtualTexture& rhs) = delete;
		VirtualTexture& operator=(const VirtualTexture& rhs) = delete;

		VirtualTexture(VirtualTexture&& rhs) noexcept = default;
		VirtualTexture& operator=(VirtualTexture&& rhs) noexcept = default;

		std::uint32_t GetVirtualTextureID() const;

	private:
		void ReserveGPUSceneVirtualTextureDescription();

	private:
		std::unique_ptr<D3D12::Texture2D> mIndirectionTexture;
		FilePathHash mBVTXFileHash;
		D3D12::StructuredBufferSubAllocation<VirtualTextureDescription, 1> mDescriptionSubAllocation;
		GPUSceneBufferUpdater<GPUSceneBufferID::VIRTUAL_TEXTURE_DESCRIPTION_BUFFER> mDescriptionBufferUpdater;
	};
}