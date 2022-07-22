module;
#include <memory>
#include <cassert>
#include <atomic>
#include <optional>
#include <DxDef.h>

module Brawler.VirtualTexture;
import Brawler.GPUSceneManager;
import Brawler.D3D12.Texture2DBuilder;
import Brawler.D3D12.GPUResourceSpecialInitializationMethod;
import Brawler.VirtualTextureConstants;

namespace
{
	static constexpr std::uint64_t DELETION_FLAG = (static_cast<std::uint64_t>(1) << 63);

	consteval Brawler::D3D12::RenderTargetTexture2DBuilder CreateDefaultIndirectionTextureBuilder()
	{
		// We need to clear indirection textures to 0s before we can use them. To do
		// that, we must create them as render targets and use 
		// ID3D12GraphicsCommandList::ClearRenderTargetView().
		//
		// We can make use of AMD's fast clears by doing this, which, according to the
		// information at https://gpuopen.com/performance/, are much faster than filling
		// the full target with, e.g., a copy.
		
		Brawler::D3D12::RenderTargetTexture2DBuilder indirectionTextureBuilder{};

		// The format for indirection textures is DXGI_FORMAT_R8G8B8A8_UINT. The channels
		// are assigned as follows:
		//
		//  - R8: Global Texture Page X Coordinates
		//  - G8: Global Texture Page Y Coordinates
		//  - B8: Global Texture Description Buffer Index
		//  - A8: Bit Flag
		//    * Lower 1 Bit:
		//      - True: This page has a valid allocation.
		//      - False: This page has no allocation.
		// 
		//    * Upper 7 Bits: Reserved - Must Be Zeroed
		static constexpr DXGI_FORMAT INDIRECTION_TEXTURE_FORMAT = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;
		indirectionTextureBuilder.SetTextureFormat(INDIRECTION_TEXTURE_FORMAT);

		static constexpr D3D12_CLEAR_VALUE INDIRECTION_TEXTURE_OPTIMIZED_CLEAR_VALUE{
			.Format = INDIRECTION_TEXTURE_FORMAT,
			.Color{ 0.0f, 0.0f, 0.0f, 0.0f }
		};
		indirectionTextureBuilder.SetOptimizedClearValue(INDIRECTION_TEXTURE_OPTIMIZED_CLEAR_VALUE);

		// The D3D12 API requires that render targets and depth/stencil textures be cleared, copied
		// into, or discarded as their first use on the GPU. The Brawler Engine handles this
		// automatically by considering the GPUResourceSpecialInitializationMethod of the I_GPUResource.
		//
		// By default, the Brawler Engine will automatically discard the resource through a call
		// to ID3D12GraphicsCommandList::DiscardResource(). However, since we need the texture
		// to be cleared anyways, we can just set the preferred special initialization method to
		// GPUResourceSpecialInitializationMethod::CLEAR. This will automatically clear the resource
		// on its first use on the GPU via a call to ID3D12GraphicsCommandList::ClearRenderTargetView().
		//
		// This is why you do not see a clear operation being done on the first use of an indirection
		// texture. (This is also why you do not see the initial resource state being set for the
		// indirection texture: The Brawler Engine assumes that render targets are in the
		// D3D12_RESOURCE_STATE_RENDER_TARGET state as their initial resource state in order to perform
		// special initialization. An assert is fired in Debug builds if this is not the case, but
		// the Texture2DBuilder classes will always choose the correct initial state if needed.)
		indirectionTextureBuilder.SetPreferredSpecialInitializationMethod(Brawler::D3D12::GPUResourceSpecialInitializationMethod::CLEAR);

		// This is all of the information which we can set at compile time.
		return indirectionTextureBuilder.
	}
}

namespace Brawler
{
	VirtualTexture::VirtualTexture(const FilePathHash bvtxFileHash) :
		mIndirectionTexture(),
		mIndirectionTextureBindlessAllocation(),
		mBVTXFileHash(bvtxFileHash),
		mDescriptionSubAllocation(),
		mDescriptionBufferUpdater(),
		mMetadata(),
		mStreamingRequestsInFlight(0)
	{
		ReserveGPUSceneVirtualTextureDescription();
		mMetadata.InitializeFromVirtualTextureFile(mBVTXFileHash);
		InitializeIndirectionTexture();
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
		assert(descriptionSubAllocation.has_value() && "ERROR: We have run out of virtual texture slots in the GPUScene buffer!");

		mDescriptionSubAllocation = std::move(*descriptionSubAllocation);
		mDescriptionBufferUpdater = GPUSceneBufferUpdater<GPUSceneBufferID::VIRTUAL_TEXTURE_DESCRIPTION_BUFFER>{ mDescriptionSubAllocation.GetBufferCopyRegion() };
	}

	void VirtualTexture::InitializeIndirectionTexture()
	{
		static constexpr D3D12::RenderTargetTexture2DBuilder DEFAULT_INDIRECTION_TEXTURE_BUILDER{ CreateDefaultIndirectionTextureBuilder() };

		// In the original implementation of virtual textures, each virtual texture has a mip-mapped
		// indirection texture which has one texel for every page and one mip level for every logical
		// mip level of the virtual texture. However, this type of set up does not support mip levels
		// of virtual textures whose dimensions are smaller than that of a page.
		//
		// For the Brawler Engine, we use an indirection texture to represent each mip level which
		// is *NOT* found in the combined page. To get the page coordinates within the global texture
		// for the combined page of a virtual texture, we dynamically branch to instead look into
		// the VirtualTextureDescription buffer.
		//
		// Another idea I had for this was to lay out the indirection texture in a manner similar
		// to the combined page such that it can represent every mip level inside of one texture.
		// By doing this, you reduce the potential for divergence within a wave, but you also lose
		// access to hardware mip level selection for indirection for all mip levels not found
		// in the combined page.
		//
		// In practice, assuming that lanes within a wave are logically located adjacent to each
		// other, I don't expect divergence to be too bad with the current set up.


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