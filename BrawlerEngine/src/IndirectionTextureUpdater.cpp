module;
#include <cstdint>
#include <vector>
#include <cassert>
#include <DxDef.h>

module Brawler.VirtualTextureManagementSubModule;
import Util.Math;
import Util.D3D12;
import Brawler.GlobalTextureReservedPage;
import Brawler.D3D12.Texture2D;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureLogicalPage;
import Brawler.VirtualTextureMetadata;

namespace
{
	// We use a DXGI_FORMAT_R8G8B8A8_UINT format indirection texture. The channels are defined as
	// specified in the comments of <anonymous namespace>::CreateDefaultIndirectionTextureBuilder(),
	// which is found in VirtualTexture.cpp.
	static constexpr DXGI_FORMAT INDIRECTION_TEXTURE_FORMAT = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;

	static constexpr std::size_t NUM_BYTES_PER_INDIRECTION_TEXEL = 4;

	// Each page of a virtual texture in a mip level not found in the combined page is represented
	// by a 2x2 quad in the indirection texture. This is done to make room for an additional mip
	// level in the indirection texture which describes the coordinates of the combined page within
	// a global texture.
	static constexpr std::size_t REQUIRED_SIZE_FOR_LARGE_PAGE_INDIRECTION_TEXTURE_CHANGE = (Util::Math::AlignToPowerOfTwo(NUM_BYTES_PER_INDIRECTION_TEXEL * 2, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) + (NUM_BYTES_PER_INDIRECTION_TEXEL * 2));

	// The combined page, however, only has a single texel in the last mip level of the indirection
	// texture.
	static constexpr std::size_t REQUIRED_SIZE_FOR_COMBINED_PAGE_INDIRECTION_TEXTURE_CHANGE = NUM_BYTES_PER_INDIRECTION_TEXEL;

	consteval Util::D3D12::CopyableFootprint GetIndirectionTexturePageFootprint(const std::uint32_t pageDimensions)
	{
		return Util::D3D12::CopyableFootprint{
			.Layout{
				.Offset = 0,
				.Footprint{
					.Format = INDIRECTION_TEXTURE_FORMAT,
					.Width = pageDimensions,
					.Height = pageDimensions,
					.Depth = 1,
					.RowPitch = Util::Math::AlignToPowerOfTwo(NUM_BYTES_PER_INDIRECTION_TEXEL * pageDimensions, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
				}
			},
			.NumRows = pageDimensions,
			.RowSizeInBytes = (NUM_BYTES_PER_INDIRECTION_TEXEL * pageDimensions)
		};
	}

	static constexpr Util::D3D12::CopyableFootprint LARGE_PAGE_INDIRECTION_TEXTURE_FOOTPRINT{ GetIndirectionTexturePageFootprint(2) };
	static constexpr Util::D3D12::CopyableFootprint COMBINED_PAGE_INDIRECTION_TEXTURE_FOOTPRINT{ GetIndirectionTexturePageFootprint(1) };
}

namespace Brawler
{
	void IndirectionTextureUpdater::AddUpdatesForPageSwapOperation(const GlobalTexturePageSwapOperation& pageSwapOperation)
	{
		AddReplacementPageUpdateInfo(pageSwapOperation);

		// TODO: Add the UpdateInfo for the page being replaced, but only if applicable.
	}

	void IndirectionTextureUpdater::AddReplacementPageUpdateInfo(const GlobalTexturePageSwapOperation& pageSwapOperation)
	{
		// Align the current size to D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT to make room in the
		// buffer for the next sub-resource copy. (All texture copy data for a given sub-resource
		// needs to be aligned to this value in the D3D12 API.)
		mRequiredUploadBufferSize = Util::Math::AlignToPowerOfTwo(mRequiredUploadBufferSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

		const GlobalTextureReservedPage& replacementPage{ pageSwapOperation.GetReplacementPage() };
		const VirtualTexturePageType replacementPageType = replacementPage.GetAllocatedPageType();

		const bool isCombinedPage = (replacementPageType == VirtualTexturePageType::COMBINED_PAGE);

		D3D12::Texture2D& destinationIndirectionTexture{ replacementPage.GetVirtualTexture().GetIndirectionTexture() };

		const VirtualTextureLogicalPage& logicalPage{ replacementPage.GetAllocatedLogicalPage() };

		const std::uint32_t relevantIndirectionTextureMipLevel = std::clamp<std::uint32_t>(logicalPage.LogicalMipLevel, 0, replacementPage.GetVirtualTexture().GetVirtualTextureMetadata().GetFirstMipLevelInCombinedPage());
		const Texture2DSubResource destinationSubResource{ destinationIndirectionTexture.GetSubResource(relevantIndirectionTextureMipLevel) };

		const std::int32_t indirectionTextureStartCoordX = static_cast<std::int32_t>(replacementPage.GetAllocatedLogicalPage().LogicalPageXCoordinate * (isCombinedPage ? 1 : 2));
		const std::int32_t indirectionTextureStartCoordY = static_cast<std::int32_t>(replacementPage.GetAllocatedLogicalPage().LogicalPageYCoordinate * (isCombinedPage ? 1 : 2));

		D3D12::TextureCopyRegion indirectionTextureCopyRegion{ destinationSubResource, CD3DX12_BOX{
			indirectionTextureStartCoordX,
			indirectionTextureStartCoordY,
			(indirectionTextureStartCoordX + (isCombinedPage ? 1 : 2)),
			(indirectionTextureStartCoordY + (isCombinedPage ? 1 : 2))
		} };

		// Ensure that the page dimensions can be described in 8 bits. This should always be the
		// case, and the indirection texture's 8-bit channels relies on this behavior. Also, ensure
		// that we can also fit the global texture description buffer index in 8 bits.
		assert(pageSwapOperation.GetGlobalTextureXPageCoordinates() <= std::numeric_limits<std::uint8_t>::max());
		assert(pageSwapOperation.GetGlobalTextureYPageCoordinates() <= std::numeric_limits<std::uint8_t>::max());
		assert(pageSwapOperation.GetGlobalTextureDescriptionBufferIndex() <= std::numeric_limits<std::uint8_t>::max());

		// Create the value which will be copied into the relevant texels of the indirection
		// texture.
		const std::uint32_t newTexelValue = ((pageSwapOperation.GetGlobalTextureXPageCoordinates() << 24) | (pageSwapOperation.GetGlobalTextureYPageCoordinates() << 16) | (pageSwapOperation.GetGlobalTextureDescriptionBufferIndex() << 8) | 0x1);

		UpdateInfo replacementPageUpdateInfo{
			.DestinationCopyRegion{ std::move(indirectionTextureCopyRegion) },
			.SourceCopySubAllocation{ (isCombinedPage ? COMBINED_PAGE_INDIRECTION_TEXTURE_FOOTPRINT : LARGE_PAGE_INDIRECTION_TEXTURE_FOOTPRINT) },
			.IndirectionTextureTexelValue = newTexelValue
		};
		mRequiredUploadBufferSize += replacementPageUpdateInfo.SourceCopySubAllocation.GetSubAllocationSize();

		mUpdateInfoArr.push_back(std::move(replacementPageUpdateInfo));
	}
}