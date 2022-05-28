module;
#include <array>
#include <vector>
#include <span>
#include <atomic>
#include <cassert>
#include <DxDef.h>

module Brawler.AssetManagement.Win32AssetIORequestBuilder;
import Brawler.D3D12.BufferResource;
import Brawler.AssetManagement.BPKArchiveReader;
import Brawler.AssetManagement.ZSTDDecompressionOperation;
import Util.General;

namespace Brawler
{
	namespace AssetManagement
	{
		Win32AssetIORequestBuilder::Win32AssetIORequestBuilder(AssetRequestEventHandle&& hAssetRequestEvent) :
			mRequestContainerArr(),
			mRequestTracker(std::move(hAssetRequestEvent))
		{}
		
		void Win32AssetIORequestBuilder::AddAssetIORequest(const Brawler::FilePathHash pathHash, Brawler::D3D12::I_BufferSubAllocation& bufferSubAllocation)
		{
			assert(bufferSubAllocation.GetBufferResource().GetHeapType() == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD && "ERROR: An attempt was made to write asset data into an I_BufferSubAllocation whose associated BufferResource was not located in an UPLOAD heap!");

			Win32AssetIORequest assetIORequest{ pathHash, mRequestTracker };
			assetIORequest.SetWriteDataCallback([pathHash, &bufferSubAllocation] (const std::span<const std::byte> srcDataSpan)
			{
				// UPLOAD heaps are located in write-combined memory, which is inherently slow to read from. We
				// expect ZStandard decompression to do a lot of reading from the destination data, so if the
				// data is compressed, then we will first decompress into a temporary byte array and then copy
				// the decompressed data into the buffer.

				const BPKArchiveReader::TOCEntry& tocEntry{ BPKArchiveReader::GetInstance().GetTableOfContentsEntry(pathHash) };

				if (tocEntry.IsDataCompressed())
				{
					ZSTDDecompressionOperation decompressionOperation{};
					Util::General::CheckHRESULT(decompressionOperation.BeginDecompressionOperation(srcDataSpan));

					const ZSTDDecompressionOperation::DecompressionResults decompressResults{ decompressionOperation.FinishDecompressionOperation() };
					Util::General::CheckHRESULT(decompressResults.HResult);

					bufferSubAllocation.WriteToBuffer(std::span<const std::byte>{ decompressResults.DecompressedByteArr }, 0);
				}
				else
				{
					// On the other hand, if the data is not compressed, it is much more efficient to just copy all of
					// it to the GPU immediately than it is to first copy it to the CPU and then again to the GPU.

					bufferSubAllocation.WriteToBuffer(srcDataSpan, 0);
				}
			});

			GetCurrentRequestContainer().push_back(std::move(assetIORequest));
		}

		std::span<Win32AssetIORequest> Win32AssetIORequestBuilder::GetAssetIORequestSpan(const Brawler::JobPriority priority)
		{
			return std::span<Win32AssetIORequest>{ mRequestContainerArr[std::to_underlying(priority)] };
		}

		std::span<const Win32AssetIORequest> Win32AssetIORequestBuilder::GetAssetIORequestSpan(const Brawler::JobPriority priority) const
		{
			return std::span<const Win32AssetIORequest>{ mRequestContainerArr[std::to_underlying(priority)] };
		}

		void Win32AssetIORequestBuilder::Finalize()
		{
			std::size_t numRequests = 0;

			for (const auto& requestContainer : mRequestContainerArr)
				numRequests += requestContainer.size();

			mRequestTracker.SetActiveRequestCount(static_cast<std::uint32_t>(numRequests));
		}

		bool Win32AssetIORequestBuilder::ReadyForDeletion() const
		{
			return mRequestTracker.IsAssetRequestEventComplete();
		}

		Win32AssetIORequestBuilder::RequestContainer& Win32AssetIORequestBuilder::GetCurrentRequestContainer()
		{
			return mRequestContainerArr[std::to_underlying(GetAssetIORequestPriority())];
		}

		const Win32AssetIORequestBuilder::RequestContainer& Win32AssetIORequestBuilder::GetCurrentRequestContainer() const
		{
			return mRequestContainerArr[std::to_underlying(GetAssetIORequestPriority())];
		}
	}
}