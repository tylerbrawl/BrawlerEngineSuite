module;
#include <memory>
#include <vector>
#include <cassert>
#include <optional>
#include <ranges>
#include <filesystem>
#include <span>
#include <DxDef.h>

module Brawler.GlobalTextureUploadBuffer;
import Brawler.GlobalTextureReservedPage;
import Brawler.VirtualTextureMetadata;
import Brawler.VirtualTexture;
import Util.Math;
import Util.Engine;
import Util.General;
import Brawler.D3D12.PersistentGPUResourceManager;
import Brawler.D3D12.BufferResourceInitializationInfo;
import Brawler.D3D12.TextureCopyRegion;
import Brawler.D3D12.Texture2D;
import Brawler.AssetManagement.AssetDependency;
import Brawler.AssetManagement.I_AssetIORequestBuilder;
import Brawler.AssetManagement.BPKArchiveReader;
import Brawler.AssetManagement.AssetManager;
import Brawler.FilePathHash;

// Wanna know something weird? This builds, even though Brawler.D3D12.TextureCopyBufferSubAllocation isn't
// imported. Go figure.

namespace Brawler
{
	void GlobalTextureUploadBuffer::AddPageSwapOperation(std::unique_ptr<GlobalTexturePageSwapOperation>&& pageSwapOperation)
	{
		assert(mUploadBufferPtr == nullptr && "ERROR: An attempt was made to call GlobalTextureUploadBuffer::AddPageSwapOperation() after calling GlobalTextureUploadBuffer::BeginTextureDataStreaming()!");
		assert(pageSwapOperation != nullptr);

		mPageSwapOperationPtrArr.push_back(std::move(pageSwapOperation));
	}

	void GlobalTextureUploadBuffer::BeginTextureDataStreaming()
	{
		// Get the total required size of this upload buffer. We do this by adding all of the
		// copyable footprints sizes of each page which is to be streamed into the global texture.
		//
		// However, care must be taken to align the data of each page with
		// D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT.

		assert(mUploadBufferPtr == nullptr && "ERROR: GlobalTextureUploadBuffer::BeginTextureDataStreaming() was called more than once for a GlobalTextureUploadBuffer instance!");
		assert(!mPageSwapOperationPtrArr.empty() && "ERROR: An attempt was made to call GlobalTextureUploadBuffer::BeginTextureDataStreaming() before any GlobalTexturePageSwapOperations were added to the GlobalTextureUploadBuffer instance!");

		std::size_t totalUploadBufferSize = 0;

		{
			const VirtualTextureMetadata& firstPageVTMetadata{ mPageSwapOperationPtrArr[0]->GetReplacementPage().GetVirtualTexture().GetVirtualTextureMetadata() };
			totalUploadBufferSize += firstPageVTMetadata.GetCopyableFootprintsPageSize();
		}

		for (const auto& pageSwapOperationPtr : mPageSwapOperationPtrArr | std::views::drop(1))
		{
			totalUploadBufferSize = Util::Math::AlignToPowerOfTwo(totalUploadBufferSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
			
			const VirtualTextureMetadata& currVTMetadata{ pageSwapOperationPtr->GetReplacementPage().GetVirtualTexture().GetVirtualTextureMetadata() };
			totalUploadBufferSize += currVTMetadata.GetCopyableFootprintsPageSize();
		}

		mUploadBufferPtr = std::make_unique<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
			.SizeInBytes = totalUploadBufferSize,
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD
		});

		// Assign each GlobalTexturePageSwapOperation instance a TextureCopyBufferSubAllocation which
		// is given a reservation into the upload buffer which we just created.
		for (const auto& pageSwapOperationPtr : mPageSwapOperationPtrArr)
		{
			// Create the TextureCopyRegion instance representing the segment of the new page's
			// GlobalTexture.
			D3D12::TextureCopyRegion globalTextureCopyRegion{ pageSwapOperationPtr->GetGlobalTexture().GetSubResource(0), pageSwapOperationPtr->GetGlobalTextureCopyRegionBox() };

			std::optional<D3D12::TextureCopyBufferSubAllocation> globalTextureCopySubAllocation{ mUploadBufferPtr->CreateBufferSubAllocation<D3D12::TextureCopyBufferSubAllocation>(globalTextureCopyRegion) };
			assert(globalTextureCopySubAllocation.has_value());

			pageSwapOperationPtr->AssignGlobalTextureCopySubAllocation(std::move(*globalTextureCopySubAllocation));
		}

		// We actually need to immediately create the ID3D12Resource instance, since this buffer
		// is not necessarily going to be used with the FrameGraph before the asset management
		// system gets a hold of it. 
		// 
		// (Under most circumstances, the FrameGraph will automatically detect which I_GPUResource 
		// instances are being used and allocate ID3D12Resource instances for them.)
		Util::General::CheckHRESULT(Util::Engine::GetPersistentGPUResourceManager().AllocatePersistentGPUResource(*mUploadBufferPtr));

		// Create a new AssetDependency instance which will load the texture data. Internally, the
		// AssetManager will use DirectStorage to load the texture data, unless it finds for some
		// reason that this API cannot be used; in that case, it will gracefully fall back to the
		// Win32 API.
		AssetManagement::AssetDependency textureDataDependency{};
		textureDataDependency.AddAssetDependencyResolver([this] (AssetManagement::I_AssetIORequestBuilder& requestBuilder)
		{
			const std::filesystem::path& bpkArchivePath{ AssetManagement::BPKArchiveReader::GetInstance().GetBPKArchiveFilePath() };

			for (const auto& pageSwapOperationPtr : mPageSwapOperationPtrArr)
			{
				const VirtualTexture& currPageVirtualTexture{ pageSwapOperationPtr->GetReplacementPage().GetVirtualTexture() };

				// Get the offset from the start of the BPK archive to the BVTX file.
				std::size_t currPageOffsetFromBPKArchiveStart = AssetManagement::BPKArchiveReader::GetInstance().GetTableOfContentsEntry(currPageVirtualTexture.GetBVTXFilePathHash()).FileOffsetInBytes;

				const VirtualTexturePageMetadata& currPageMetadata{ pageSwapOperationPtr->GetReplacementPage().GetAllocatedPageMetadata() };

				// Add the offset from the start of the BVTX file to the start of the compressed
				// page data to get the offset from the start of the BPK archive to the start
				// of the compressed page data.
				currPageOffsetFromBPKArchiveStart += currPageMetadata.OffsetFromBVTXFileStartToPageData;

				const AssetManagement::CustomFileAssetIORequest pageDataRequest{
					.FilePath{ bpkArchivePath },
					.FileOffset{ currPageOffsetFromBPKArchiveStart },
					.CompressedDataSizeInBytes = currPageMetadata.CompressedSize,
					.UncompressedDataSizeInBytes = currPageVirtualTexture.GetVirtualTextureMetadata().GetCopyableFootprintsPageSize()
				};

				// Submit the request for DirectStorage to process.
				requestBuilder.AddAssetIORequest(pageDataRequest, pageSwapOperationPtr->GetGlobalTextureCopySubAllocation());
			}
		});

		// Send the AssetDependency to the AssetManager. Eventually, the request will be asynchronously
		// fulfilled, and we will be able to continue the GlobalTexture update process. We will know when
		// this is the case by checking mHPageDataLoadEvent.
		mHPageDataLoadEvent = AssetManagement::AssetManager::GetInstance().EnqueueAssetDependency(std::move(textureDataDependency));
	}

	bool GlobalTextureUploadBuffer::IsTextureDataPrepared() const
	{
		return mHPageDataLoadEvent.IsAssetRequestComplete();
	}

	std::span<const std::unique_ptr<GlobalTexturePageSwapOperation>> GlobalTextureUploadBuffer::GetPageSwapOperationSpan() const
	{
		return std::span<const std::unique_ptr<GlobalTexturePageSwapOperation>>{ mPageSwapOperationPtrArr };
	}
}