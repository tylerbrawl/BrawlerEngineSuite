module;
#include <mutex>
#include <vector>
#include <span>
#include <memory>
#include <cassert>
#include <ranges>
#include <array>
#include <DxDef.h>

module Brawler.VirtualTextureManagementSubModule;
import Util.D3D12;
import Brawler.VirtualTextureLogicalPage;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureMetadata;
import Brawler.GlobalTexturePageInfo;
import Util.Math;
import Brawler.D3D12.BufferResource;
import Brawler.GlobalTexturePageIdentifier;
import Brawler.D3D12.Texture2D;

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
	
	static constexpr Util::D3D12::CopyableFootprint INDIRECTION_TEXTURE_COMBINED_PAGE_COPYABLE_FOOTPRINT_RESULTS{ CalculateIndirectionTextureFootprints<1>().FootprintsArr[0] };
	static constexpr Util::D3D12::CopyableFootprint INDIRECTION_TEXTURE_LARGE_PAGE_COPYABLE_FOOTPRINT_RESULTS{ CalculateIndirectionTextureFootprints<2>().FootprintsArr[0] };
}

namespace Brawler
{
	void GlobalTexturePageTransferSubModule::AddPageTransferRequests(const std::span<std::unique_ptr<GlobalTexturePageTransferRequest>> transferRequestSpan)
	{
		const std::scoped_lock<std::mutex> lock{ mCritSection };

		for (auto&& transferRequestPtr : transferRequestSpan)
			mTransferRequestPtrArr.push_back(std::move(transferRequestPtr));
	}

	bool GlobalTexturePageTransferSubModule::HasPageTransferRequests() const
	{
		const std::scoped_lock<std::mutex> lock{ mCritSection };
		return !mTransferRequestPtrArr.empty();
	}

	std::vector<GlobalTexturePageTransferSubModule::PageTransferRenderPass_T> GlobalTexturePageTransferSubModule::GetPageTransferRenderPasses(D3D12::FrameGraphBuilder& builder)
	{
		std::vector<std::unique_ptr<GlobalTexturePageTransferRequest>> currRequestArr{};

		{
			const std::scoped_lock<std::mutex> lock{ mCritSection };
			currRequestArr = std::move(mTransferRequestPtrArr);
		}

		if (currRequestArr.empty())
			return std::vector<std::unique_ptr<GlobalTexturePageTransferRequest>>{};

		// In order to copy page data between GlobalTextures, we could potentially make use of
		// ID3D12GraphicsCommandList::CopyTextureRegion() to copy between GlobalTextures, rather
		// than use a working buffer. However, there are some issues with this approach:
		//
		//   - Suppose we are transferring from one location within a GlobalTexture to another
		//     location within the same GlobalTexture. We wouldn't be able to do the copy like
		//     that because we would not be able to put the GlobalTexture in both the
		//     D3D12_RESOURCE_STATE_COPY_SOURCE and the D3D12_RESOURCE_STATE_COPY_DEST states,
		//     so we would have to use a buffer. (Admittedly, this case would be quite pointless
		//     for decreasing memory consumption, since GlobalTextures do not suffer from internal
		//     fragmentation. However, moving texels within a GlobalTexture closer to each other
		//     could potentially improve cache hit rates during texture sampling.)
		//
		//   - 
	}

	std::vector<GlobalTexturePageTransferSubModule::IndirectionTextureUpdateRenderPass_T> GlobalTexturePageTransferSubModule::CreateIndirectionTextureUpdateRenderPasses(D3D12::FrameGraphBuilder& builder, const std::span<const std::unique_ptr<GlobalTexturePageTransferRequest>> transferRequestSpan)
	{
		assert(!transferRequestSpan.empty());
		
		std::vector<IndirectionTextureUpdatePassInfo> passInfoArr{};
		std::size_t requiredUploadBufferSize = 0;

		for (const auto& transferRequestPtr : transferRequestSpan)
		{
			Util::Math::AlignToPowerOfTwo(requiredUploadBufferSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

			const D3D12::TextureCopyRegion& destinationCopyRegion{ transferRequestPtr->GetDestinationGlobalTexturePageInfo().GetPageCopyRegion() };
			
			// Determine if we are dealing with a large page or a combined page. Large pages need to take
			// four texels in the indirection texture, while the combined page only needs one.
			const VirtualTextureLogicalPage& logicalPage{ transferRequestPtr->GetLogicalPage() };

			assert(logicalPage.VirtualTexturePtr != nullptr);
			const bool isCombinedPage = (logicalPage.LogicalMipLevel >= logicalPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetFirstMipLevelInCombinedPage());

			IndirectionTextureUpdatePassInfo currPassInfo{
				.DestCopyRegion{ destinationCopyRegion }
			};

			if (isCombinedPage)
				currPassInfo.SrcDataBufferSubAllocation = D3D12::TextureCopyBufferSubAllocation{ INDIRECTION_TEXTURE_COMBINED_PAGE_COPYABLE_FOOTPRINT_RESULTS };
			else
				currPassInfo.SrcDataBufferSubAllocation = D3D12::TextureCopyBufferSubAllocation{ INDIRECTION_TEXTURE_LARGE_PAGE_COPYABLE_FOOTPRINT_RESULTS };

			requiredUploadBufferSize += currPassInfo.SrcDataBufferSubAllocation.GetSubAllocationSize();
			passInfoArr.push_back(std::move(currPassInfo));
		}

		D3D12::BufferResource& uploadBuffer{ builder.CreateTransientResource<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
			.SizeInBytes = requiredUploadBufferSize,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
		}) };

		// Rather than create a single RenderPass containing all of the indirection texture updates, we
		// create one RenderPass per update. This consumes more memory within the Brawler Engine's state
		// tracking system, but it allows for more effective use of split barriers and better command list
		// recording parallelism.
		std::vector<IndirectionTextureUpdateRenderPass_T> updatePassArr{};

		for (auto& [transferRequestPtr, passInfo] : std::views::zip(transferRequestSpan, passInfoArr))
		{
			const bool wasAssignmentSuccessful = uploadBuffer.AssignReservation(passInfo.SrcDataBufferSubAllocation);
			assert(wasAssignmentSuccessful);

			const GlobalTexturePageIdentifier destinationPageIdentifier{ transferRequestPtr->GetDestinationGlobalTexturePageInfo().GetPageIdentifier() };

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
			const std::uint32_t newIndirectionTextureValue = ((static_cast<std::uint32_t>(destinationPageIdentifier.GlobalTexturePageXCoordinate) << 24) | (static_cast<std::uint32_t>(destinationPageIdentifier.GlobalTexturePageYCoordinate) << 16) | (static_cast<std::uint32_t>(destinationPageIdentifier.GlobalTextureID) << 8) | 0x1);
			const std::array<std::uint32_t, 2> valueCopyArr{ newIndirectionTextureValue, newIndirectionTextureValue };

			// If the row count is one, then we are dealing with a combined page; otherwise, we are dealing
			// with a large page.
			const bool isCombinedPage = (passInfo.SrcDataBufferSubAllocation.GetRowCount() == 1);

			if (isCombinedPage)
			{
				const std::span<std::uint32_t> singleTexelSpan{ valueCopyArr.data(), 1 };
				passInfo.SrcDataBufferSubAllocation.WriteTextureData(0, singleTexelSpan);
			}
			else
			{
				assert(passInfo.SrcDataBufferSubAllocation.GetRowCount() == 2);

				const std::span<std::uint32_t> rowDataSpan{ valueCopyArr.data(), 2 };
				passInfo.SrcDataBufferSubAllocation.WriteTextureData(0, rowDataSpan);
				passInfo.SrcDataBufferSubAllocation.WriteTextureData(1, rowDataSpan);
			}

			IndirectionTextureUpdateRenderPass_T indirectionTextureUpdatePass{};
			indirectionTextureUpdatePass.SetRenderPassName("Virtual Texture Management: GlobalTexture Page Transfers - Indirection Texture Update Pass");

			const D3D12::Texture2DSubResource destinationSubResource{ transferRequestPtr->GetLogicalPage().VirtualTexturePtr->GetIndirectionTexture().GetSubResource(0) };

			indirectionTextureUpdatePass.AddResourceDependency(destinationSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
			indirectionTextureUpdatePass.AddResourceDependency(passInfo.SrcDataBufferSubAllocation, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

			indirectionTextureUpdatePass.SetInputData(std::move(passInfo));

			indirectionTextureUpdatePass.SetRenderPassCommands([] (D3D12::DirectContext& context, const IndirectionTextureUpdatePassInfo& passInfo)
			{
				const D3D12::TextureCopyBufferSnapshot srcDataBufferSnapshot{ passInfo.SrcDataBufferSubAllocation };
				context.CopyBufferToTexture(passInfo.DestCopyRegion, srcDataBufferSnapshot);
			});

			updatePassArr.push_back(std::move(indirectionTextureUpdatePass));
		}

		return updatePassArr;
	}
}