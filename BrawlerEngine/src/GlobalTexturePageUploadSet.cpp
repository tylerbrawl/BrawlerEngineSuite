module;
#include <vector>
#include <memory>
#include <optional>
#include <filesystem>
#include <span>
#include <cassert>
#include <DxDef.h>

module Brawler.GlobalTexturePageUploadSet;
import Brawler.VirtualTextureLogicalPage;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureMetadata;
import Util.Math;
import Brawler.AssetManagement.AssetDependency;
import Brawler.AssetManagement.AssetManager;
import Brawler.AssetManagement.I_AssetIORequestBuilder;
import Brawler.FilePathHash;
import Brawler.AssetManagement.BPKArchiveReader;

namespace Brawler
{
	void GlobalTexturePageUploadSet::AddPageUploadRequest(std::unique_ptr<GlobalTexturePageUploadRequest>&& uploadRequestPtr)
	{
		mRequestPtrArr.push_back(std::move(uploadRequestPtr));
	}

	void GlobalTexturePageUploadSet::PrepareRequestedPageData()
	{
		assert(mUploadBufferPtr == nullptr && "ERROR: An attempt was made to call GlobalTexturePageUploadSet::PrepareRequestedPageData() more than once on the same GlobalTexturePageUploadSet instance!");

		if (!HasActiveUploadRequests()) [[unlikely]]
			return;

		std::size_t requiredBufferSize = 0;

		for (const auto& requestPtr : mRequestPtrArr)
		{
			requiredBufferSize = Util::Math::AlignToPowerOfTwo(requiredBufferSize, requestPtr->GetPageDataBufferSubAllocation().GetRequiredDataPlacementAlignment());
			requiredBufferSize += requestPtr->GetPageDataBufferSubAllocation().GetSubAllocationSize();
		}

		mUploadBufferPtr = std::make_unique<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
			.SizeInBytes = requiredBufferSize,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
		});

		AssetManagement::AssetDependency pageDataAssetDependency{};

		for (const auto& requestPtr : mRequestPtrArr)
		{
			const bool wasReservationSuccessful = mUploadBufferPtr->AssignReservation(requestPtr->GetPageDataBufferSubAllocation());
			assert(wasReservationSuccessful);

			const VirtualTextureLogicalPage& relevantLogicalPage{ requestPtr->GetLogicalPage() };
			const VirtualTexture& relevantVirtualTexture{ *(relevantLogicalPage.VirtualTexturePtr) };

			const FilePathHash bvtxFilePathHash{ relevantVirtualTexture.GetBVTXFilePathHash() };
			const AssetManagement::BPKArchiveReader::TOCEntry& bvtxTOCEntry{ AssetManagement::BPKArchiveReader::GetInstance().GetTableOfContentsEntry(bvtxFilePathHash) };
			assert(!bvtxTOCEntry.IsDataCompressed() && "ERROR: BVTX files should not be compressed themselves! (They are composed of lightweight page descriptions and page texture data compressed with zstandard.)");

			// We want the AssetDependency to be for the actual page data. This is not necessarily located at the beginning
			// of a BVTX file. So, in order to make use of the asset streaming system, we need to manually calculate the
			// offset to the start of the relevant data. This can be done by adding the offset from the start of the BVTX
			// file to the actual page data to the offset from the start of the BPK archive to the BVTX file. This gives
			// us the offset from the start of the BVTX file to the compressed page data.
			const VirtualTexturePageMetadata& relevantPageMetadata{ relevantVirtualTexture.GetVirtualTextureMetadata().GetPageMetadata(relevantLogicalPage.LogicalMipLevel, relevantLogicalPage.LogicalPageXCoordinate, relevantLogicalPage.LogicalPageYCoordinate) };
			const std::size_t offsetFromBPKStartToPageData = (bvtxTOCEntry.FileOffsetInBytes + relevantPageMetadata.OffsetFromBVTXFileStartToPageData);

			struct RequestData
			{
				std::size_t FileOffset;
				std::size_t CompressedDataSizeInBytes;
				std::size_t UncompressedDataSizeInBytes;
			};

			RequestData currRequestData{
				.FileOffset = offsetFromBPKStartToPageData,
				.CompressedDataSizeInBytes = relevantPageMetadata.CompressedSize,
				.UncompressedDataSizeInBytes = relevantVirtualTexture.GetVirtualTextureMetadata().GetCopyableFootprintsPageSize()
			};

			pageDataAssetDependency.AddAssetDependencyResolver([requestData = std::move(currRequestData), rawRequestPtr = requestPtr.get()] (AssetManagement::I_AssetIORequestBuilder& requestBuilder)
			{
				const AssetManagement::CustomFileAssetIORequest customFileRequest{
					.FilePath{ AssetManagement::BPKArchiveReader::GetBPKArchiveFilePath() },
					.FileOffset = requestData.FileOffset,
					.CompressedDataSizeInBytes = requestData.CompressedDataSizeInBytes,
					.UncompressedDataSizeInBytes = requestData.UncompressedDataSizeInBytes
				};

				requestBuilder.AddAssetIORequest(customFileRequest, rawRequestPtr->GetPageDataBufferSubAllocation());
			});
		}

		mHPageDataUploadEvent = AssetManagement::AssetManager::GetInstance().EnqueueAssetDependency(std::move(pageDataAssetDependency));
	}

	bool GlobalTexturePageUploadSet::HasActiveUploadRequests() const
	{
		return !mRequestPtrArr.empty();
	}

	bool GlobalTexturePageUploadSet::ReadyForGlobalTextureUploads() const
	{
		if (!HasActiveUploadRequests()) [[unlikely]]
			return true;

		assert(mUploadBufferPtr != nullptr && "ERROR: An attempt was made to call GlobalTexturePageUploadSet::ReadyForGlobalTextureUploads() before GlobalTexturePageUploadSet::PrepareRequestedPageData() was called!");
		return mHPageDataUploadEvent.IsAssetRequestComplete();
	}

	std::span<const std::unique_ptr<GlobalTexturePageUploadRequest>> GlobalTexturePageUploadSet::GetUploadRequestSpan() const
	{
		return std::span<const std::unique_ptr<GlobalTexturePageUploadRequest>>{ mRequestPtrArr };
	}

	std::unique_ptr<D3D12::BufferResource> GlobalTexturePageUploadSet::ExtractUploadBufferResource()
	{
		return std::move(mUploadBufferPtr);
	}
}