module;
#include <memory>
#include <cassert>
#include <atomic>
#include <optional>
#include <DxDef.h>

module Brawler.VirtualTexture;
import Brawler.GPUSceneManager;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.D3D12.GPUResourceSpecialInitializationMethod;
import Brawler.VirtualTextureConstants;
import Brawler.D3D12.ShaderResourceView;
import Util.Math;
import Util.Engine;
import Brawler.GPUSceneBufferUpdater;
import Brawler.GPUSceneBufferID;

namespace
{
	static constexpr std::uint64_t NOT_READY_FRAME_NUMBER = 0;
	static constexpr DXGI_FORMAT INDIRECTION_TEXTURE_FORMAT = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;

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
		indirectionTextureBuilder.SetTextureFormat(INDIRECTION_TEXTURE_FORMAT);

		constexpr D3D12_CLEAR_VALUE INDIRECTION_TEXTURE_OPTIMIZED_CLEAR_VALUE{
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
		return indirectionTextureBuilder;
	}
}

namespace Brawler
{
	VirtualTexture::VirtualTexture(const FilePathHash bvtxFileHash) :
		mIndirectionTexture(),
		mIndirectionTextureBindlessAllocation(),
		mBVTXFileHash(bvtxFileHash),
		mDescriptionSubAllocation(),
		mMetadata(),

		// We initialize mStreamingRequestsInFlight to 1, rather than 0, so that we do not need
		// to perform an additional atomic operation for the mandatory combined page streaming.
		mStreamingRequestsInFlight(1),

		mFirstAvailableFrameNumber(NOT_READY_FRAME_NUMBER)
	{
		mMetadata.InitializeFromVirtualTextureFile(mBVTXFileHash);
		InitializeIndirectionTexture();
		InitializeGPUSceneVirtualTextureDescription();
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

	D3D12::Texture2D& VirtualTexture::GetIndirectionTexture()
	{
		assert(mIndirectionTexture != nullptr);
		return *mIndirectionTexture;
	}

	const D3D12::Texture2D& VirtualTexture::GetIndirectionTexture() const
	{
		assert(mIndirectionTexture != nullptr);
		return *mIndirectionTexture;
	}

	bool VirtualTexture::ReadyForUse() const
	{
		const std::uint64_t firstAvailableFrameNumber = mFirstAvailableFrameNumber.load(std::memory_order::relaxed);

		if (firstAvailableFrameNumber == NOT_READY_FRAME_NUMBER) [[unlikely]]
			return false;

		return (Util::Engine::GetTrueFrameNumber() >= firstAvailableFrameNumber);
	}
	
	void VirtualTexture::InitializeIndirectionTexture()
	{
		static constexpr D3D12::RenderTargetTexture2DBuilder DEFAULT_INDIRECTION_TEXTURE_BUILDER{ CreateDefaultIndirectionTextureBuilder() };

		// In the original implementation of virtual textures, each virtual texture has a mip-mapped
		// indirection texture which has one texel for every page and one mip level for every logical
		// mip level of the virtual texture. However, this type of set up does not support mip levels
		// of virtual textures whose dimensions are smaller than that of a page.
		//
		// For the Brawler Engine, we also use an indirection texture, but the layout is slightly
		// different. The very last mip level in the mip chain of the indirection texture will describe
		// the information of the combined page, and every other mip level will describe the corresponding
		// logical mip level of the virtual texture which does not fit in the combined page. These other
		// mip levels will need a 2x2 quad to represent each page of the logical mip level, since we
		// need to double the dimensions of the indirection texture in order to add a mip level for
		// the combined page.

		// Normally, to calculate the dimensions of the top mip level of the indirection texture, we
		// would do (MipLevel0Dimensions / UnpaddedPageDimensions) * 2. This can be optimized as
		// (MipLevel0Dimensions / (UnpaddedPageDimensions / 2)), where (UnpaddedPageDimensions / 2) can
		// be calculated at compile time.
		static constexpr std::uint32_t HALF_UNPADDED_PAGE_DIMENSIONS = (UNPADDED_VIRTUAL_TEXTURE_PAGE_SIZE / 2);
		const std::uint32_t indirectionTextureMip0Dimensions = (mMetadata.GetLogicalMipLevel0Dimensions() / HALF_UNPADDED_PAGE_DIMENSIONS);

		D3D12::RenderTargetTexture2DBuilder indirectionTextureBuilder{ DEFAULT_INDIRECTION_TEXTURE_BUILDER };
		indirectionTextureBuilder.SetTextureDimensions(indirectionTextureMip0Dimensions, indirectionTextureMip0Dimensions);

		const std::uint32_t indirectionTextureMipLevelCount = (mMetadata.GetFirstMipLevelInCombinedPage() + 1);
		indirectionTextureBuilder.SetMipLevelCount(static_cast<std::uint16_t>(indirectionTextureMipLevelCount));

		mIndirectionTexture = std::make_unique<D3D12::Texture2D>(indirectionTextureBuilder);

		// Reserve a bindless SRV for the indirection texture.
		const Brawler::D3D12::Texture2DShaderResourceView<INDIRECTION_TEXTURE_FORMAT> indirectionTextureSRV{ *mIndirectionTexture, D3D12_TEX2D_SRV{
			.MostDetailedMip = 0,
			.MipLevels = indirectionTextureMipLevelCount,
			.PlaneSlice = 0,
			.ResourceMinLODClamp = 0.0f
		} };
		mIndirectionTextureBindlessAllocation = static_cast<D3D12::I_GPUResource&>(*mIndirectionTexture).CreateBindlessSRV(indirectionTextureSRV.CreateSRVDescription());
	}

	void VirtualTexture::InitializeGPUSceneVirtualTextureDescription()
	{
		// Get a sub-allocation (and, more importantly, a reservation) from the GPUSceneBuffer for
		// virtual texture descriptions. If we fail to do this, then we have run out of virtual texture
		// description memory.
		std::optional<D3D12::StructuredBufferSubAllocation<VirtualTextureDescription, 1>> descriptionSubAllocation{ GPUSceneManager::GetInstance().GetGPUSceneBufferResource<GPUSceneBufferID::VIRTUAL_TEXTURE_DESCRIPTION_BUFFER>().CreateBufferSubAllocation<D3D12::StructuredBufferSubAllocation<VirtualTextureDescription, 1>>() };
		assert(descriptionSubAllocation.has_value() && "ERROR: We have run out of virtual texture slots in the GPUScene buffer!");

		mDescriptionSubAllocation = std::move(*descriptionSubAllocation);

		assert(Util::Math::IsPowerOfTwo(mMetadata.GetLogicalMipLevel0Dimensions()));
		const std::uint32_t log2VTSize = std::countr_zero(mMetadata.GetLogicalMipLevel0Dimensions());
		assert(log2VTSize <= std::numeric_limits<std::uint8_t>::max());

		const std::uint32_t packedIndirectionTextureIndexAndLog2VTSize = ((mIndirectionTextureBindlessAllocation.GetBindlessSRVIndex() << 8) | log2VTSize);

		// Set the new value for the VirtualTextureDescription within the GPUScene buffer.
		const GPUSceneBufferUpdater<GPUSceneBufferID::VIRTUAL_TEXTURE_DESCRIPTION_BUFFER> vtDescriptionBufferUpdater{ mDescriptionSubAllocation.GetBufferCopyRegion() };
		vtDescriptionBufferUpdater.UpdateGPUSceneData(VirtualTextureDescription{
			.IndirectionTextureIndexAndLog2VTSize = packedIndirectionTextureIndexAndLog2VTSize
		});
	}

	void VirtualTexture::SetFirstUseableFrameNumber(const std::uint64_t frameNumber)
	{
		mFirstAvailableFrameNumber.store(frameNumber, std::memory_order::relaxed);
	}

	void VirtualTexture::IncrementStreamingRequestCount()
	{
		mStreamingRequestsInFlight.fetch_add(1, std::memory_order::relaxed);
	}

	void VirtualTexture::DecrementStreamingRequestCount()
	{
		mStreamingRequestsInFlight.fetch_sub(1, std::memory_order::relaxed);
	}

	bool VirtualTexture::SafeToDelete() const
	{
		// Do not delete this VirtualTexture instance unless there are no pending streaming requests.

		return (mStreamingRequestsInFlight.load(std::memory_order::relaxed) == 0);
	}
}