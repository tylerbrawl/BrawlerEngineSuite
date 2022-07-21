module;
#include <memory>
#include <cassert>
#include <atomic>
#include <optional>

module Brawler.VirtualTexture;
import Brawler.GPUSceneManager;

namespace
{
	static constexpr std::uint64_t DELETION_FLAG = (static_cast<std::uint64_t>(1) << 63);
}

namespace Brawler
{
	VirtualTexture::VirtualTexture(const FilePathHash bvtxFileHash) :
		mIndirectionTexture(),
		mBVTXFileHash(bvtxFileHash),
		mDescriptionSubAllocation(),
		mDescriptionBufferUpdater(),
		mMetadata(),
		mStreamingRequestsInFlight(0)
	{
		ReserveGPUSceneVirtualTextureDescription();
		mMetadata.InitializeFromVirtualTextureFile(mBVTXFileHash);
	}

	std::uint32_t VirtualTexture::GetVirtualTextureID() const
	{
		// The ID of a VirtualTexture instance is inferred based on its location within the global
		// VirtualTextureDescription GPUSceneBuffer.
		return static_cast<std::uint32_t>(mDescriptionSubAllocation.GetOffsetFromBufferStart() / sizeof(VirtualTextureDescription));
	}

	const VirtualTextureMetadata& VirtualTexture::GetVirtualTextureMetadata() const
	{
		return mMetadata;
	}

	FilePathHash VirtualTexture::GetBVTXFilePathHash() const
	{
		return mBVTXFileHash;
	}

	void VirtualTexture::ReserveGPUSceneVirtualTextureDescription()
	{
		// Get a sub-allocation (and, more importantly, a reservation) from the GPUSceneBuffer for
		// virtual texture descriptions. If we fail to do this, then we have run out of virtual texture
		// description memory.
		std::optional<D3D12::StructuredBufferSubAllocation<VirtualTextureDescription, 1>> descriptionSubAllocation{ GPUSceneManager::GetInstance().GetGPUSceneBufferResource<GPUSceneBufferID::VIRTUAL_TEXTURE_DESCRIPTION_BUFFER>().CreateBufferSubAllocation<D3D12::StructuredBufferSubAllocation<VirtualTextureDescription, 1>>() };
		assert(descriptionSubAllocation.has_value() && "ERROR: We have run out of virtual texture description memory!");

		mDescriptionSubAllocation = std::move(*descriptionSubAllocation);
		mDescriptionBufferUpdater = GPUSceneBufferUpdater<GPUSceneBufferID::VIRTUAL_TEXTURE_DESCRIPTION_BUFFER>{ mDescriptionSubAllocation.GetBufferCopyRegion() };
	}

	void VirtualTexture::MarkForDeletion()
	{
		// The idea is that we don't want to create any page streaming requests for this VirtualTexture
		// instance after an attempt was made to delete it. We can avoid using a std::mutex by using
		// an atomic bitwise-OR operation.
		
		mStreamingRequestsInFlight.fetch_or(DELETION_FLAG, std::memory_order::relaxed);
	}

	bool VirtualTexture::SafeToDelete() const
	{
		// Do not delete this VirtualTexture instance unless we have marked it for deletion and there
		// are no pending streaming requests.

		return (mStreamingRequestsInFlight.load(std::memory_order::relaxed) == DELETION_FLAG);
	}
}