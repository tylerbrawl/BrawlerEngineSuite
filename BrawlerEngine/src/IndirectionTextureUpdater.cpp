module;
#include <cstdint>
#include <vector>
#include <array>
#include <cassert>
#include <ranges>
#include <DxDef.h>

module Brawler.VirtualTextureManagementSubModule;
import Util.Math;
import Util.D3D12;
import Brawler.GlobalTextureReservedPage;
import Brawler.D3D12.Texture2D;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureLogicalPage;
import Brawler.VirtualTextureMetadata;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.BufferResourceInitializationInfo;

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

		if (pageSwapOperation.IsReplacingOlderPage()) [[unlikely]]
			AddOldPageUpdateInfo(pageSwapOperation);
	}

	IndirectionTextureUpdater::IndirectionTextureUpdatePass_T IndirectionTextureUpdater::CreateIndirectionTextureUpdatesRenderPass(D3D12::FrameGraphBuilder& builder)
	{
		assert(mRequiredUploadBufferSize > 0 && "ERROR: IndirectionTextureUpdater::CreateIndirectionTextureUpdatesRenderPass() was called even though no indirection texture updates needed to be done!");
		assert(!mUpdateInfoArr.empty());

		AllocateDataUploadBuffer(builder);

		IndirectionTextureUpdatePass_T indirectionTextureUpdatePass{};
		indirectionTextureUpdatePass.SetRenderPassName("Virtual Texture Management - Indirection Texture Updates Pass");

		// Since each TextureCopyBufferSubAllocation within the UpdateInfo instances of mUpdateInfoArr
		// reference the same BufferResource, we only need to supply one of them as a resource
		// dependency.
		indirectionTextureUpdatePass.AddResourceDependency(mUpdateInfoArr[0].SourceCopySubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

		for (auto& updateInfo : mUpdateInfoArr)
			indirectionTextureUpdatePass.AddResourceDependency(updateInfo.DestinationSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

		indirectionTextureUpdatePass.SetInputData(IndirectionTextureUpdatePassInfo{
			.UpdateInfoArr{ std::move(mUpdateInfoArr) }
		});

		indirectionTextureUpdatePass.SetRenderPassCommands([] (D3D12::DirectContext& context, const IndirectionTextureUpdatePassInfo& passInfo)
		{
			for (const auto& updateInfo : passInfo.UpdateInfoArr)
			{
				const D3D12::TextureCopyBufferSnapshot srcCopyBufferSnapshot{ updateInfo.SourceCopySubAllocation };
				context.CopyBufferToTexture(updateInfo.DestinationCopyRegion, srcCopyBufferSnapshot);
			}
		});

		return indirectionTextureUpdatePass;
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
		D3D12::Texture2DSubResource destinationSubResource{ destinationIndirectionTexture.GetSubResource(relevantIndirectionTextureMipLevel) };

		const std::int32_t indirectionTextureStartCoordX = static_cast<std::int32_t>(logicalPage.LogicalPageXCoordinate * (isCombinedPage ? 1 : 2));
		const std::int32_t indirectionTextureStartCoordY = static_cast<std::int32_t>(logicalPage.LogicalPageYCoordinate * (isCombinedPage ? 1 : 2));

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
		// texture. (NOTE: The 0x1 is meant to indicate that the corresponding page has, in fact,
		// been uploaded into the corresponding global texture. Without this, an inactive page in
		// the indirection texture whose R, G, and B channels are all zero could be interpreted as
		// an active page located at (0, 0) in the global texture with ID 0.)
		const std::uint32_t newTexelValue = ((pageSwapOperation.GetGlobalTextureXPageCoordinates() << 24) | (pageSwapOperation.GetGlobalTextureYPageCoordinates() << 16) | (pageSwapOperation.GetGlobalTextureDescriptionBufferIndex() << 8) | 0x1);

		UpdateInfo replacementPageUpdateInfo{
			.DestinationSubResource{ std::move(destinationSubResource) },
			.DestinationCopyRegion{ std::move(indirectionTextureCopyRegion) },
			.SourceCopySubAllocation{ (isCombinedPage ? COMBINED_PAGE_INDIRECTION_TEXTURE_FOOTPRINT : LARGE_PAGE_INDIRECTION_TEXTURE_FOOTPRINT) },
			.IndirectionTextureTexelValue = newTexelValue
		};
		mRequiredUploadBufferSize += replacementPageUpdateInfo.SourceCopySubAllocation.GetSubAllocationSize();

		mUpdateInfoArr.push_back(std::move(replacementPageUpdateInfo));
	}

	void IndirectionTextureUpdater::AddOldPageUpdateInfo(const GlobalTexturePageSwapOperation& pageSwapOperation)
	{
		assert(pageSwapOperation.IsReplacingOlderPage() && "ERROR: IndirectionTextureUpdater::AddOldPageUpdateInfo() was called even though the provided GlobalTexturePageSwapOperation instance was not describing any page being replaced!");
		
		// Align the current size to D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT to make room in the
		// buffer for the next sub-resource copy. (All texture copy data for a given sub-resource
		// needs to be aligned to this value in the D3D12 API.)
		mRequiredUploadBufferSize = Util::Math::AlignToPowerOfTwo(mRequiredUploadBufferSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

		const GlobalTextureReservedPage& oldPage{ pageSwapOperation.GetPreviousPage() };
		assert(oldPage.GetAllocatedPageType == VirtualTexturePageType::OTHER && "ERROR: Combined pages should *NEVER* be swapped out for another page as part of a GlobalTexturePageSwapOperation! (They should only be vacated from a global texture once their VirtualTexture instance has been destroyed!)");

		D3D12::Texture2D& destinationIndirectionTexture{ oldPage.GetVirtualTexture().GetIndirectionTexture() };

		const VirtualTextureLogicalPage& logicalPage{ oldPage.GetAllocatedLogicalPage() };

		// Since we know that we are not dealing with a combined page here, some of the logic is
		// simplified compared to IndirectionTextureUpdater::AddReplacementPageUpdateInfo().

		D3D12::Texture2DSubResource destinationSubResource{ destinationIndirectionTexture.GetSubResource(logicalPage.LogicalMipLevel) };

		const std::int32_t indirectionTextureStartCoordX = static_cast<std::int32_t>(logicalPage.LogicalPageXCoordinate * 2);
		const std::int32_t indirectionTextureStartCoordY = static_cast<std::int32_t>(logicalPage.LogicalPageYCoordinate * 2);

		D3D12::TextureCopyRegion indirectionTextureCopyRegion{ destinationSubResource, CD3DX12_BOX{
			indirectionTextureStartCoordX,
			indirectionTextureStartCoordY,
			(indirectionTextureStartCoordX + 2),
			(indirectionTextureStartCoordY + 2)
		} };

		// We replace the entire texel with zeroes. (Strictly speaking, we only have to zero the flag
		// indicating that the page is no longer allocated, but since we have to do updates in entire
		// texels anyways, we might as well just use zero.)
		UpdateInfo oldPageUpdateInfo{
			.DestinationSubResource{ std::move(destinationSubResource) },
			.DestinationCopyRegion{ std::move(indirectionTextureCopyRegion) },
			.SourceCopySubAllocation{ LARGE_PAGE_INDIRECTION_TEXTURE_FOOTPRINT },
			.IndirectionTextureTexelValue = 0
		};
		mRequiredUploadBufferSize += oldPageUpdateInfo.SourceCopySubAllocation.GetSubAllocationSize();

		mUpdateInfoArr.push_back(std::move(oldPageUpdateInfo));
	}

	void IndirectionTextureUpdater::AllocateDataUploadBuffer(D3D12::FrameGraphBuilder& builder)
	{
		D3D12::BufferResource& uploadBuffer{ builder.CreateTransientResource<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
			.SizeInBytes = mRequiredUploadBufferSize,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
		}) };

		// For each UpdateInfo instance in mUpdateInfoArr, ...
		for (auto& updateInfo : mUpdateInfoArr)
		{
			// ...assign its SourceCopySubAllocation instance a reservation from uploadBuffer...
			const bool reservationResult = uploadBuffer.AssignReservation(updateInfo.SourceCopySubAllocation);
			assert(reservationResult && "ERROR: An attempt to reserve memory from an upload buffer created for an IndirectionTextureUpdater failed, even though the required size should have been correctly specified!");

			// ...and write the new texel data into the BufferResource.
			const std::size_t unpaddedRowSize = updateInfo.SourceCopySubAllocation.GetRowSizeInBytes();
			const std::uint32_t rowCount = updateInfo.SourceCopySubAllocation.GetRowCount();

			const std::size_t numTexelsPerRow = (unpaddedRowSize / sizeof(std::uint32_t));
			assert(numTexelsPerRow > 0);

			// The number of texels per row should never be larger than 
			// LARGE_PAGE_INDIRECTION_TEXTURE_FOOTPRINT.Layout.Footprint.Width. Knowing this, we
			// can use a std::array instead of a std::vector and avoid doing multiple small heap
			// allocations.
			static constexpr std::size_t MAX_EXPECTED_TEXELS_PER_ROW = LARGE_PAGE_INDIRECTION_TEXTURE_FOOTPRINT.Layout.Footprint.Width;
			assert(numTexelsPerRow <= MAX_EXPECTED_TEXELS_PER_ROW);
			
			std::array<std::uint32_t, MAX_EXPECTED_TEXELS_PER_ROW> rowDataArr{ updateInfo.IndirectionTextureTexelValue, updateInfo.IndirectionTextureTexelValue };
			
			// Create a differently-sized std::span based on how many texels are in a row for this
			// UpdateInfo instance's footprint.
			const std::span<const std::uint32_t> indirectionTextureRowDataSpan{ rowDataArr.data(), numTexelsPerRow };

			for (const auto i : std::views::iota(0u, rowCount))
				updateInfo.WriteTextureData(i, indirectionTextureRowDataSpan);
		}
	}
}