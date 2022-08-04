module;
#include <memory>
#include <atomic>

export module Brawler.VirtualTexture;
import Brawler.FilePathHash;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneTypes;
import Brawler.D3D12.BindlessSRVAllocation;
import Brawler.VirtualTextureMetadata;

namespace Brawler
{
	class VirtualTextureDatabase;
	class VirtualTextureManagementSubModule;
}

export namespace Brawler
{
	class VirtualTexture
	{
	private:
		friend class VirtualTextureDatabase;
		friend class VirtualTextureManagementSubModule;

	public:
		explicit VirtualTexture(const FilePathHash bvtxFileHash);

		VirtualTexture(const VirtualTexture& rhs) = delete;
		VirtualTexture& operator=(const VirtualTexture& rhs) = delete;

		VirtualTexture(VirtualTexture&& rhs) noexcept = default;
		VirtualTexture& operator=(VirtualTexture&& rhs) noexcept = default;

		std::uint32_t GetVirtualTextureID() const;
		const VirtualTextureMetadata& GetVirtualTextureMetadata() const;
		FilePathHash GetBVTXFilePathHash() const;

		D3D12::Texture2D& GetIndirectionTexture();
		const D3D12::Texture2D& GetIndirectionTexture() const;

		bool ReadyForUse() const;

	private:
		void InitializeIndirectionTexture(); 
		void InitializeGPUSceneVirtualTextureDescription();

		void SetFirstUseableFrameNumber(const std::uint64_t frameNumber);

		void IncrementStreamingRequestCount();
		void DecrementStreamingRequestCount();

		bool SafeToDelete() const;

	private:
		std::unique_ptr<D3D12::Texture2D> mIndirectionTexture;
		D3D12::BindlessSRVAllocation mIndirectionTextureBindlessAllocation;
		FilePathHash mBVTXFileHash;
		D3D12::StructuredBufferSubAllocation<VirtualTextureDescription, 1> mDescriptionSubAllocation;
		VirtualTextureMetadata mMetadata;
		std::atomic<std::uint64_t> mStreamingRequestsInFlight;
		std::atomic<std::uint64_t> mFirstAvailableFrameNumber;
	};
}