module;
#include <cstdint>
#include <array>
#include <vector>
#include <span>
#include <ranges>
#include <cassert>
#include <algorithm>
#include <DxDef.h>

module Brawler.IndirectionTextureUpdater;
import Util.Math;
import Util.D3D12;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureMetadata;
import Brawler.D3D12.Texture2D;
import Brawler.GlobalTexturePageIdentifier;
import Brawler.D3D12.BufferResource;

namespace
{
	static constexpr DXGI_FORMAT INDIRECTION_TEXTURE_FORMAT = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;

	template <std::size_t PageDimensions>
	consteval Util::D3D12::CopyableFootprintsResults CalculateIndirectionTextureFootprints()
	{
		const Brawler::D3D12_RESOURCE_DESC combinedPageIndirectionTexelDescription{
			.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = 0,
			.Width = PageDimensions,
			.Height = PageDimensions,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = INDIRECTION_TEXTURE_FORMAT,
			.SampleDesc{
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
			.SamplerFeedbackMipRegion{}
		};

		const Util::D3D12::CopyableFootprintsParams params{
			.ResourceDesc{ combinedPageIndirectionTexelDescription },
			.FirstSubResource = 0,
			.NumSubResources = 1,
			.BaseOffset = 0
		};

		return Util::D3D12::GetCopyableFootprints(params);
	}

	static constexpr Util::D3D12::CopyableFootprint INDIRECTION_TEXTURE_COMBINED_PAGE_COPYABLE_FOOTPRINT{ CalculateIndirectionTextureFootprints<1>().FootprintsArr[0] };
	static constexpr Util::D3D12::CopyableFootprint INDIRECTION_TEXTURE_LARGE_PAGE_COPYABLE_FOOTPRINT{ CalculateIndirectionTextureFootprints<2>().FootprintsArr[0] };
}

namespace Brawler
{
	void IndirectionTextureUpdater::UpdateIndirectionTextureEntry(const VirtualTextureLogicalPage& logicalPage, const GlobalTexturePageInfo& destinationPageInfo)
	{
		const GlobalTexturePageIdentifier destinationPageIdentifier{ destinationPageInfo.GetPageIdentifier() };

		// The value assigned to the indirection texture texel(s) encodes the following information in
		// R8G8B8A8_UINT format:
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
		//
		// See <anonymous namespace>::CreateDefaultIndirectionTextureBuilder() in VirtualTexture.cpp for more details.
		const std::uint32_t newIndirectionTexelValue = ((static_cast<std::uint32_t>(destinationPageIdentifier.GlobalTexturePageXCoordinate) << 24) | (static_cast<std::uint32_t>(destinationPageIdentifier.GlobalTexturePageYCoordinate) << 16) | (static_cast<std::uint32_t>(destinationPageIdentifier.GlobalTextureID) << 8) | 0x1);

		AddUpdateInfo(logicalPage, newIndirectionTexelValue);
	}

	void IndirectionTextureUpdater::ClearIndirectionTextureEntry(const VirtualTextureLogicalPage& logicalPage)
	{
		AddUpdateInfo(logicalPage, 0);
	}

	std::vector<IndirectionTextureUpdater::IndirectionTextureUpdateRenderPass_T> IndirectionTextureUpdater::FinalizeIndirectionTextureUpdates(D3D12::FrameGraphBuilder& builder)
	{
		D3D12::BufferResource& uploadBuffer{ builder.CreateTransientResource<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
			.SizeInBytes = mCurrRequiredBufferSize,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
		}) };

		std::vector<IndirectionTextureUpdateRenderPass_T> updatePassArr{};

		for (auto&& updateInfoStorage : mUpdateInfoStorageArr)
		{
			const bool wasReservationSuccessful = uploadBuffer.AssignReservation(updateInfoStorage.PassInfo.SrcSubAllocation);
			assert(wasReservationSuccessful);

			const std::uint32_t rowCount = updateInfoStorage.PassInfo.SrcSubAllocation.GetRowCount();
			const std::span<const std::uint32_t> rowTextureDataSpan{ updateInfoStorage.IndirectionTexelValueArr.data(), rowCount };

			for (const auto i : std::views::iota(0u, rowCount))
				updateInfoStorage.PassInfo.SrcSubAllocation.WriteTextureData(i, rowTextureDataSpan);

			IndirectionTextureUpdateRenderPass_T currUpdatePass{};
			currUpdatePass.SetRenderPassName("Virtual Texture Management - Indirection Texture Update Pass");

			currUpdatePass.AddResourceDependency(updateInfoStorage.IndirectionTextureSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
			currUpdatePass.AddResourceDependency(updateInfoStorage.PassInfo.SrcSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

			currUpdatePass.SetInputData(std::move(updateInfoStorage.PassInfo));

			currUpdatePass.SetRenderPassCommands([] (D3D12::DirectContext& context, const IndirectionTextureUpdatePassInfo& passInfo)
			{
				const D3D12::TextureCopyBufferSnapshot srcSnapshot{ passInfo.SrcSubAllocation };
				context.CopyBufferToTexture(passInfo.DestCopyRegion, srcSnapshot);
			});

			updatePassArr.push_back(std::move(currUpdatePass));
		}

		mUpdateInfoStorageArr.clear();
		mCurrRequiredBufferSize = 0;

		return updatePassArr;
	}

	void IndirectionTextureUpdater::AddUpdateInfo(const VirtualTextureLogicalPage& logicalPage, const std::uint32_t newIndirectionTexelValue)
	{
		assert(logicalPage.VirtualTexturePtr != nullptr);

		mCurrRequiredBufferSize = Util::Math::AlignToPowerOfTwo(mCurrRequiredBufferSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
		IndirectionTextureUpdatePassInfo currUpdatePassInfo{
			.DestCopyRegion{ logicalPage.GetIndirectionTextureCopyRegion() }
		};

		const std::uint32_t firstMipLevelInCombinedPage = logicalPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetFirstMipLevelInCombinedPage();
		const std::uint32_t clampedMipLevel = std::clamp<std::uint32_t>(logicalPage.LogicalMipLevel, 0u, firstMipLevelInCombinedPage);

		const bool isCombinedPage = (clampedMipLevel == firstMipLevelInCombinedPage);

		if (isCombinedPage)
			currUpdatePassInfo.SrcSubAllocation = D3D12::TextureCopyBufferSubAllocation{ INDIRECTION_TEXTURE_COMBINED_PAGE_COPYABLE_FOOTPRINT };
		else
			currUpdatePassInfo.SrcSubAllocation = D3D12::TextureCopyBufferSubAllocation{ INDIRECTION_TEXTURE_LARGE_PAGE_COPYABLE_FOOTPRINT };

		mCurrRequiredBufferSize += currUpdatePassInfo.SrcSubAllocation.GetSubAllocationSize();
		mUpdateInfoStorageArr.push_back(UpdateInfoStorage{
			.PassInfo{ std::move(currUpdatePassInfo) },
			.IndirectionTextureSubResource{ logicalPage.VirtualTexturePtr->GetIndirectionTexture().GetSubResource(clampedMipLevel) },
			.IndirectionTexelValueArr{ newIndirectionTexelValue, newIndirectionTexelValue }
		});
	}
}