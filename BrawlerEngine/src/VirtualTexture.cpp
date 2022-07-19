module;
#include <memory>
#include <cassert>
#include <optional>

module Brawler.VirtualTexture;
import Brawler.GPUSceneManager;

namespace Brawler
{
	VirtualTexture::VirtualTexture(const FilePathHash bvtxFileHash) :
		mIndirectionTexture(),
		mBVTXFileHash(bvtxFileHash),
		mDescriptionSubAllocation(),
		mDescriptionBufferUpdater()
	{
		ReserveGPUSceneVirtualTextureDescription();
	}

	std::uint32_t VirtualTexture::GetVirtualTextureID() const
	{
		// The ID of a VirtualTexture instance is inferred based on its location within the global
		// VirtualTextureDescription GPUSceneBuffer.
		return (mDesscriptionSubAllocation.GetOffsetFromBufferStart() / sizeof(VirtualTextureDescription));
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
}