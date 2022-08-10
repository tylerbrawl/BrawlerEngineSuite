module;
#include <mutex>
#include <vector>
#include <span>
#include <memory>
#include <cassert>
#include <ranges>
#include <array>
#include <algorithm>
#include <DxDef.h>

module Brawler.GlobalTexturePageTransferSubModule;
import Util.D3D12;
import Util.Math;
import Brawler.VirtualTextureLogicalPage;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureMetadata;
import Brawler.GlobalTexturePageInfo;
import Brawler.D3D12.BufferResource;
import Brawler.GlobalTexturePageIdentifier;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.JobSystem;
import Brawler.OptionalRef;

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

	static constexpr Brawler::D3D12::Texture2DBuilder GLOBAL_TEXTURE_PAGE_TEMPORARY_COPY_BUILDER{ []()
	{
		Brawler::D3D12::Texture2DBuilder temporaryCopyBuilder{};
		temporaryCopyBuilder.SetMipLevelCount(1);
		temporaryCopyBuilder.DenyUnorderedAccessViews();
		temporaryCopyBuilder.SetInitialResourceState(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

		return temporaryCopyBuilder;
	}() };
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

	GlobalTexturePageTransferSubModule::GlobalTextureTransferPassCollection GlobalTexturePageTransferSubModule::GetPageTransferRenderPasses(D3D12::FrameGraphBuilder& builder)
	{
		std::vector<std::unique_ptr<GlobalTexturePageTransferRequest>> currRequestArr{};

		{
			const std::scoped_lock<std::mutex> lock{ mCritSection };
			currRequestArr = std::move(mTransferRequestPtrArr);
		}

		// There is overhead in spawning CPU jobs, so don't bother if we don't actually have
		// any work to do.
		if (currRequestArr.empty())
			return GlobalTextureTransferPassCollection{};

		// We realize that GlobalTexture page transfers consist of two separate components:
		//
		//   - GlobalTexture Page Data Updates: The page data within the GlobalTextures
		//     is re-arranged.
		//
		//   - Indirection Texture Updates: The indirection textures of the relevant virtual
		//     textures whose pages are being moved must be updated.
		//
		// These tasks are completely independent from each other, so we can run them
		// concurrently on two separate queues on the GPU timeline. We can also create their
		// respective render passes concurrently, which further boosts parallelism. To do
		// this, we create a std::span<const std::unique_ptr<GlobalTexturePageTransferRequest>>
		// and copy this into the individual jobs. (We could pass the std::span by reference, but
		// there's no real point, since these are cheap to copy.)

		const std::span<const std::unique_ptr<GlobalTexturePageTransferRequest>> currRequestSpan{ currRequestArr };
		Brawler::JobGroup globalTexturePageTransferGroup{};
		globalTexturePageTransferGroup.Reserve(2);

		std::vector<GlobalTextureCopyRenderPass_T> globalTextureCopyArr{};
		D3D12::FrameGraphBuilder globalTextureCopyBuilder{ builder.GetFrameGraph() };
		globalTexturePageTransferGroup.AddJob([&globalTextureCopyArr, &globalTextureCopyBuilder, currRequestSpan] ()
		{
			globalTextureCopyArr = CreateGlobalTextureCopyRenderPasses(globalTextureCopyBuilder, currRequestSpan);
		});

		std::vector<IndirectionTextureUpdateRenderPass_T> indirectionTextureUpdateArr{};
		D3D12::FrameGraphBuilder indirectionTextureUpdateBuilder{ builder.GetFrameGraph() };
		globalTexturePageTransferGroup.AddJob([&indirectionTextureUpdateArr, &indirectionTextureUpdateBuilder, currRequestSpan] ()
		{
			indirectionTextureUpdateArr = CreateIndirectionTextureUpdateRenderPasses(indirectionTextureUpdateBuilder, currRequestSpan);
		});

		globalTexturePageTransferGroup.ExecuteJobs();

		builder.MergeFrameGraphBuilder(std::move(globalTextureCopyBuilder));
		builder.MergeFrameGraphBuilder(std::move(indirectionTextureUpdateBuilder));

		return GlobalTextureTransferPassCollection{
			.GlobalTextureCopyArr{ std::move(globalTextureCopyArr) },
			.IndirectionTextureUpdateArr{ std::move(indirectionTextureUpdateArr) }
		};
	}

	std::vector<GlobalTexturePageTransferSubModule::GlobalTextureCopyRenderPass_T> GlobalTexturePageTransferSubModule::CreateGlobalTextureCopyRenderPasses(D3D12::FrameGraphBuilder& builder, const std::span<const std::unique_ptr<GlobalTexturePageTransferRequest>> transferRequestSpan)
	{
		assert(!transferRequestSpan.empty());

		// To ensure the correctness of the transfer requests, we need to sort them according to their source
		// and destination GlobalTexture pages. The question arises of how we must sort them. Consider the
		// following two transfer requests:
		//
		//   - Request 1: Page A -> Page B
		//   - Request 2: Page B -> Page C
		//
		// There are two possible interpretations as to how these requests must be ordered:
		//
		//   - Request 1 goes first, meaning that the contents of A are copied into B and C. The original
		//     contents of B are lost.
		//
		//   - Request 2 goes first, meaning that the contents of C become those of B, and those of B
		//     subsequently become those of A.
		//
		// As it turns out, only the second interpretation is correct. The GlobalTextureUpdateContext - 
		// which is responsible for creating transfer requests prior to submission to the 
		// GlobalTexturePageTransferSubModule - keeps track of the final state of each VirtualTextureLogicalPage
		// after an update. This results in at most one request per VirtualTextureLogicalPage.
		//
		// We exploit this behavior to infer that if the contents of A really were supposed to be moved into
		// C, then the GlobalTextureUpdateContext would have only generated one transfer request:
		// Page A -> Page C. Since this request was never generated, and since this is the only way that
		// the contents of A would be copied into the contents of C, we conclude that the first interpretation
		// must be invalid.
		//
		// Generalizing these results to N requests, we find that we must sort transfer requests as follows:
		//
		// If a request R1 has a source GlobalTexture page P and a request R2 has P as its destination GlobalTexture
		// page, then R1 must be sequenced before R2. Otherwise, R1 and R2 may be indeterminantly sequenced.
		
		// We cannot sort the transferRequestSpan itself, nor can we sort the array it is viewing, since that
		// array is also being viewed concurrently for the creation of indirection texture update render passes.
		// So, we need to create a second array of pointers and sort that.
		std::vector<const GlobalTexturePageTransferRequest*> sortedRequestPtrArr{};

		for (const auto& requestPtr : transferRequestSpan)
			sortedRequestPtrArr.push_back(requestPtr.get());

		std::ranges::sort(sortedRequestPtrArr, [] (const GlobalTexturePageTransferRequest* const lhs, const GlobalTexturePageTransferRequest* const rhs)
		{
			// To re-iterate, here is the sorting criterion:
			//
			// If a request R1 has a source GlobalTexture page P and a request R2 has P as its destination GlobalTexture
			// page, then R1 must be sequenced before R2. Otherwise, R1 and R2 may be indeterminantly sequenced.

			if (lhs->GetSourceGlobalTexturePageInfo().ArePagesEquivalent(rhs->GetDestinationGlobalTexturePageInfo()))
				return true;

			return !(rhs->GetSourceGlobalTexturePageInfo().ArePagesEquivalent(lhs->GetDestinationGlobalTexturePageInfo()));
		});

		// There's nothing we can do about transfer requests which must be sequenced before/after each other,
		// but if we wanted to, we could re-order the indeterminantly-sequenced requests for the sake of
		// resource barrier efficiency. I'm not sure if it would be worth the effort, however, since page transfer
		// requests shouldn't be generated too often. (They typically only happen as a result of GlobalTexture
		// defragmentation.)

		std::vector<GlobalTextureCopyRenderPass_T> copyPassArr{};

		// Since we cannot put a texture into both the D3D12_RESOURCE_STATE_COPY_SOURCE state and the
		// D3D12_RESOURCE_STATE_COPY_DEST state, if the transfer happens from one page in a GlobalTexture
		// to another page within the same GlobalTexture, then we need to use a transient texture to
		// copy the data into. This case should be very uncommon, but for correctness, we need to implement
		// it. (I suppose one argument for transferring page data within a single GlobalTexture instance
		// is to move pages closer together to improve texture cache hit rates during shading, but I'm
		// not even sure how helpful that would be in practice.)

		const auto addPassBetweenTwoGlobalTexturesLambda = [&copyPassArr] (const GlobalTexturePageTransferRequest& transferRequest)
		{
			GlobalTextureCopyRenderPass_T copyPass{};
			copyPass.SetRenderPassName("Virtual Texture Management: GlobalTexture Page Transfers - GlobalTexture Page Update Pass (Multiple GlobalTextures)");

			D3D12::Texture2DSubResource destinationSubResource{ transferRequest.GetDestinationGlobalTexturePageInfo().GetGlobalTexture2D().GetSubResource(0) };
			copyPass.AddResourceDependency(destinationSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

			D3D12::Texture2DSubResource sourceSubResource{ transferRequest.GetSourceGlobalTexturePageInfo().GetGlobalTexture2D().GetSubResource(0) };
			copyPass.AddResourceDependency(sourceSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

			copyPass.SetInputData(GlobalTextureCopyPassInfo{
				.DestCopyRegion{ transferRequest.GetDestinationGlobalTexturePageInfo().GetPageCopyRegion() },
				.SrcCopyRegion{ transferRequest.GetSourceGlobalTexturePageInfo().GetPageCopyRegion() }
			});

			copyPass.SetRenderPassCommands([] (D3D12::CopyContext& context, const GlobalTextureCopyPassInfo& passInfo)
			{
				context.CopyTextureToTexture(passInfo.DestCopyRegion, passInfo.SrcCopyRegion);
			});

			copyPassArr.push_back(std::move(copyPass));
		};

		const auto addPassWithinSingleGlobalTextureLambda = [&copyPassArr, &builder] (const GlobalTexturePageTransferRequest& transferRequest)
		{
			// Initialize the temporary copy texture's Texture2DBuilder with compile-time defaults.
			D3D12::Texture2DBuilder temporaryCopyTextureBuilder{ GLOBAL_TEXTURE_PAGE_TEMPORARY_COPY_BUILDER };
			
			const CD3DX12_BOX& srcCopyRegionBox{ transferRequest.GetSourceGlobalTexturePageInfo().GetPageCopyRegion().GetCopyRegionBox() };
			const std::size_t paddedPageDimensions = (static_cast<std::size_t>(srcCopyRegionBox.right) - static_cast<std::size_t>(srcCopyRegionBox.left));

			assert(paddedPageDimensions == (static_cast<std::size_t>(srcCopyRegionBox.bottom) - static_cast<std::size_t>(srcCopyRegionBox.top)));

			temporaryCopyTextureBuilder.SetTextureDimensions(paddedPageDimensions, paddedPageDimensions);

			const DXGI_FORMAT globalTextureFormat = transferRequest.GetSourceGlobalTexturePageInfo().GetGlobalTexture2D().GetResourceDescription().Format;
			temporaryCopyTextureBuilder.SetTextureFormat(globalTextureFormat);

			D3D12::Texture2D& temporaryCopyTexture{ builder.CreateTransientResource<D3D12::Texture2D>(temporaryCopyTextureBuilder) };
			const CD3DX12_BOX temporaryCopyTextureRegionBox{
				0,
				0,
				static_cast<std::int32_t>(paddedPageDimensions),
				static_cast<std::int32_t>(paddedPageDimensions)
			};

			D3D12::Texture2DSubResource globalTextureSubResource{ transferRequest.GetSourceGlobalTexturePageInfo().GetGlobalTexture2D().GetSubResource(0) };

			// We actually need to create two passes here. One pass moves from the GlobalTexture to the temporary
			// texture, and the other does the opposite.

			{
				GlobalTextureCopyRenderPass_T copyToTemporaryPass{};
				copyToTemporaryPass.SetRenderPassName("Virtual Texture Management: GlobalTexture Page Transfers - GlobalTexture Page Update Pass (GlobalTexture to Temporary Texture)");

				D3D12::Texture2DSubResource destinationSubResource{ temporaryCopyTexture.GetSubResource(0) };
				copyToTemporaryPass.AddResourceDependency(destinationSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

				copyToTemporaryPass.AddResourceDependency(globalTextureSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

				copyToTemporaryPass.SetInputData(GlobalTextureCopyPassInfo{
					.DestCopyRegion{ destinationSubResource, temporaryCopyTextureRegionBox },
					.SrcCopyRegion{ transferRequest.GetSourceGlobalTexturePageInfo().GetPageCopyRegion() }
				});

				copyToTemporaryPass.SetRenderPassCommands([] (D3D12::CopyContext& context, const GlobalTextureCopyPassInfo& passInfo)
				{
					context.CopyTextureToTexture(passInfo.DestCopyRegion, passInfo.SrcCopyRegion);
				});

				copyPassArr.push_back(std::move(copyToTemporaryPass));
			}

			{
				GlobalTextureCopyRenderPass_T copyToGlobalTexturePass{};
				copyToGlobalTexturePass.SetRenderPassName("Virtual Texture Management: GlobalTexture Page Transfers - GlobalTexture Page Update Pass (Temporary Texture to GlobalTexture)");

				copyToGlobalTexturePass.AddResourceDependency(globalTextureSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

				D3D12::Texture2DSubResource srcSubResource{ temporaryCopyTexture.GetSubResource(0) };
				copyToGlobalTexturePass.AddResourceDependency(srcSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

				copyToGlobalTexturePass.SetInputData(GlobalTextureCopyPassInfo{
					.DestCopyRegion{ transferRequest.GetDestinationGlobalTexturePageInfo().GetPageCopyRegion() },
					.SrcCopyRegion{ srcSubResource, temporaryCopyTextureRegionBox }
				});

				copyToGlobalTexturePass.SetRenderPassCommands([] (D3D12::CopyContext& context, const GlobalTextureCopyPassInfo& passInfo)
				{
					context.CopyTextureToTexture(passInfo.DestCopyRegion, passInfo.SrcCopyRegion);
				});

				copyPassArr.push_back(std::move(copyToGlobalTexturePass));
			}
		};

		for (const auto sortedRequestPtr : sortedRequestPtrArr)
		{
			const bool doesTransferOccurAcrossGlobalTextures = (sortedRequestPtr->GetSourceGlobalTexturePageInfo().GetPageIdentifier().GlobalTextureID != sortedRequestPtr->GetDestinationGlobalTexturePageInfo().GetPageIdentifier().GlobalTextureID);

			if (doesTransferOccurAcrossGlobalTextures) [[likely]]
				addPassBetweenTwoGlobalTexturesLambda(*sortedRequestPtr);
			else [[unlikely]]
				addPassWithinSingleGlobalTextureLambda(*sortedRequestPtr);
		}

		return copyPassArr;
	}

	std::vector<GlobalTexturePageTransferSubModule::IndirectionTextureUpdateRenderPass_T> GlobalTexturePageTransferSubModule::CreateIndirectionTextureUpdateRenderPasses(D3D12::FrameGraphBuilder& builder, const std::span<const std::unique_ptr<GlobalTexturePageTransferRequest>> transferRequestSpan)
	{
		assert(!transferRequestSpan.empty());
		
		std::vector<IndirectionTextureUpdatePassInfo> passInfoArr{};
		std::size_t requiredUploadBufferSize = 0;

		for (const auto& transferRequestPtr : transferRequestSpan)
		{
			Util::Math::AlignToPowerOfTwo(requiredUploadBufferSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
			
			// Determine if we are dealing with a large page or a combined page. Large pages need to take
			// four texels in the indirection texture, while the combined page only needs one.
			const VirtualTextureLogicalPage& logicalPage{ transferRequestPtr->GetLogicalPage() };
			const D3D12::TextureCopyRegion destinationCopyRegion{ logicalPage.GetIndirectionTextureCopyRegion() };

			assert(logicalPage.VirtualTexturePtr != nullptr);
			const bool isCombinedPage = (logicalPage.LogicalMipLevel >= logicalPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetFirstMipLevelInCombinedPage());

			IndirectionTextureUpdatePassInfo currPassInfo{
				.DestCopyRegion{ std::move(destinationCopyRegion) }
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
				const std::span<const std::uint32_t> singleTexelSpan{ valueCopyArr.data(), 1 };
				passInfo.SrcDataBufferSubAllocation.WriteTextureData(0, singleTexelSpan);
			}
			else
			{
				assert(passInfo.SrcDataBufferSubAllocation.GetRowCount() == 2);

				const std::span<const std::uint32_t> rowDataSpan{ valueCopyArr.data(), 2 };
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