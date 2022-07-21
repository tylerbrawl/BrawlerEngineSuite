module;
#include <memory>
#include <atomic>

export module Brawler.VirtualTexture;
import Brawler.FilePathHash;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneTypes;
import Brawler.GPUSceneBufferUpdater;
import Brawler.GPUSceneBufferID;
import Brawler.D3D12.BindlessSRVAllocation;
import Brawler.VirtualTextureMetadata;

namespace Brawler
{
	class VirtualTextureDatabase;
}

export namespace Brawler
{
	class VirtualTexture
	{
	private:
		friend class VirtualTextureDatabase;

	public:
		explicit VirtualTexture(const FilePathHash bvtxFileHash);

		VirtualTexture(const VirtualTexture& rhs) = delete;
		VirtualTexture& operator=(const VirtualTexture& rhs) = delete;

		VirtualTexture(VirtualTexture&& rhs) noexcept = default;
		VirtualTexture& operator=(VirtualTexture&& rhs) noexcept = default;

		std::uint32_t GetVirtualTextureID() const;
		const VirtualTextureMetadata& GetVirtualTextureMetadata() const;
		FilePathHash GetBVTXFilePathHash() const;

	private:
		void ReserveGPUSceneVirtualTextureDescription();
		void InitializeIndirectionTexture();

		void MarkForDeletion();
		bool SafeToDelete() const;

	private:
		std::unique_ptr<D3D12::Texture2D> mIndirectionTexture;
		D3D12::BindlessSRVAllocation mIndirectionTextureBindlessAllocation;
		FilePathHash mBVTXFileHash;
		D3D12::StructuredBufferSubAllocation<VirtualTextureDescription, 1> mDescriptionSubAllocation;
		GPUSceneBufferUpdater<GPUSceneBufferID::VIRTUAL_TEXTURE_DESCRIPTION_BUFFER> mDescriptionBufferUpdater;
		VirtualTextureMetadata mMetadata;
		std::atomic<std::uint64_t> mStreamingRequestsInFlight;
	};
}