module;
#include <vector>
#include <array>
#include <cassert>
#include <DxDef.h>

module Brawler.AssetManagement.DirectStorageAssetIORequestBuilder;
import Brawler.AssetManagement.BPKArchiveReader;
import Brawler.D3D12.BufferResource;
import Util.DirectStorage;
import Util.General;

namespace Brawler
{
	namespace AssetManagement
	{
		void VerifyBPKAssetCompatibility(const Brawler::FilePathHash pathHash)
		{
			if constexpr (Util::General::IsDebugModeEnabled())
			{
				const BPKArchiveReader::TOCEntry& tocEntry{ BPKArchiveReader::GetInstance().GetTableOfContentsEntry(pathHash) };

				assert(tocEntry.UncompressedSizeInBytes <= Util::DirectStorage::STAGING_BUFFER_SIZE_IN_BYTES && "ERROR: The uncompressed size of a BPK asset was too large to fit into a DirectStorage staging buffer! Either increase the value of Util::DirectStorage::STAGING_BUFFER_SIZE_IN_BYTES or decompose the asset into smaller parts.");
			}
		}
	}
}

namespace Brawler
{
	namespace AssetManagement
	{
		DirectStorageAssetIORequestBuilder::DirectStorageAssetIORequestBuilder(IDStorageFile& bpkDStorageFile) :
			I_AssetIORequestBuilder(),
			mBPKDStorageFilePtr(std::addressof(bpkDStorageFile)),
			mDStorageRequestContainerArr()
		{}
		
		void DirectStorageAssetIORequestBuilder::AddAssetIORequest(const Brawler::FilePathHash pathHash, Brawler::D3D12::I_BufferSubAllocation& bufferSubAllocation)
		{
			VerifyBPKAssetCompatibility(pathHash);
			
			const BPKArchiveReader::TOCEntry& tocEntry{ BPKArchiveReader::GetInstance().GetTableOfContentsEntry(pathHash) };
			const bool isDataCompressed = tocEntry.IsDataCompressed();

			DSTORAGE_REQUEST_OPTIONS requestOptions{
				.CompressionFormat = (isDataCompressed ? DSTORAGE_COMPRESSION_FORMAT::DSTORAGE_CUSTOM_COMPRESSION_0 : DSTORAGE_COMPRESSION_FORMAT::DSTORAGE_COMPRESSION_FORMAT_NONE),
				.SourceType = DSTORAGE_REQUEST_SOURCE_TYPE::DSTORAGE_REQUEST_SOURCE_FILE,
				.DestinationType = DSTORAGE_REQUEST_DESTINATION_TYPE::DSTORAGE_REQUEST_DESTINATION_BUFFER,
				.Reserved{}
			};

			DSTORAGE_SOURCE requestSrc{ CreateDStorageSourceForBPKAsset(pathHash) };

			assert(bufferSubAllocation.GetSubAllocationSize() >= tocEntry.UncompressedSizeInBytes && "ERROR: An attempt was made to write asset data directly into an I_BufferSubAllocation, but the sub-allocation was not large enough!");
			assert(bufferSubAllocation.HasReservation() && "ERROR: An I_BufferSubAllocation instance without a BufferSubAllocationReservation (i.e., without backing GPU memory) was provided to DirectStorageAssetIORequestBuilder::AddAssetIORequest()!");

			DSTORAGE_DESTINATION requestDest{
				.Buffer{
					.Resource = &(bufferSubAllocation.GetBufferResource().GetD3D12Resource()),
					.Offset = bufferSubAllocation.GetOffsetFromBufferStart(),
					.Size = static_cast<std::uint32_t>(tocEntry.UncompressedSizeInBytes)
				}
			};

			GetCurrentRequestContainer().push_back(DSTORAGE_REQUEST{
				.Options{std::move(requestOptions)},
				.Source{ std::move(requestSrc) },
				.Destination{ std::move(requestDest) },
				.UncompressedSize = static_cast<std::uint32_t>(tocEntry.UncompressedSizeInBytes),
				.CancellationTag{},
				.Name{ nullptr }
			});
		}

		std::span<const DSTORAGE_REQUEST> DirectStorageAssetIORequestBuilder::GetDStorageRequestSpan(const Brawler::JobPriority priority) const
		{
			return std::span<const DSTORAGE_REQUEST>{ mDStorageRequestContainerArr[std::to_underlying(priority)] };
		}

		DSTORAGE_SOURCE DirectStorageAssetIORequestBuilder::CreateDStorageSourceForBPKAsset(const Brawler::FilePathHash pathHash) const
		{
			const BPKArchiveReader::TOCEntry& tocEntry{ BPKArchiveReader::GetInstance().GetTableOfContentsEntry(pathHash) };

			return DSTORAGE_SOURCE{
				.File{
					.Source = mBPKDStorageFilePtr,
					.Offset = tocEntry.FileOffsetInBytes,
					.Size = (tocEntry.IsDataCompressed() ? static_cast<std::uint32_t>(tocEntry.CompressedSizeInBytes) : static_cast<std::uint32_t>(tocEntry.UncompressedSizeInBytes))
				}
			};
		}

		DirectStorageAssetIORequestBuilder::DStorageRequestContainer& DirectStorageAssetIORequestBuilder::GetCurrentRequestContainer()
		{
			const Brawler::JobPriority currPriority = GetAssetIORequestPriority();

			assert(std::to_underlying(currPriority) < mDStorageRequestContainerArr.size());
			return mDStorageRequestContainerArr[std::to_underlying(currPriority)];
		}

		const DirectStorageAssetIORequestBuilder::DStorageRequestContainer& DirectStorageAssetIORequestBuilder::GetCurrentRequestContainer() const
		{
			const Brawler::JobPriority currPriority = GetAssetIORequestPriority();

			assert(std::to_underlying(currPriority) < mDStorageRequestContainerArr.size());
			return mDStorageRequestContainerArr[std::to_underlying(currPriority)];
		}
	}
}