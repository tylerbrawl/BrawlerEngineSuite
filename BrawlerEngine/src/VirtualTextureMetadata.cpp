module;
#include <cassert>
#include <cstdint>
#include <ranges>
#include <span>
#include <algorithm>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

module Brawler.VirtualTextureMetadata;
import Brawler.MappedFileView;
import Brawler.FileAccessMode;
import Brawler.FileMagicHandler;
import Brawler.AssetManagement.BPKArchiveReader;
import Brawler.SerializedStruct;
import Util.General;
import Util.Math;

namespace
{
	struct CommonVirtualTextureDescriptionHeader
	{
		std::uint32_t Magic;
		std::uint32_t Version;
	};

	struct VersionedVirtualTextureDescriptionHeaderV1
	{
		std::uint64_t CopyableFootprintsPageSizeInBytes;
		DXGI_FORMAT TextureFormat;
		std::uint32_t LogicalTextureMip0Dimensions;
		std::uint32_t LogicalMipLevelCount;
		std::uint32_t PageCount;
	};

	struct VersionedVirtualTextureDescriptionHeaderV2
	{
		std::uint64_t CopyableFootprintsPageSizeInBytes;
		D3D12_SUBRESOURCE_FOOTPRINT PageDataFootprint;
		std::uint32_t LogicalTextureMip0Dimensions;
		std::uint32_t LogicalMipLevelCount;
		std::uint32_t PageCount;
	};

	constexpr Brawler::FileMagicHandler VIRTUAL_TEXTURE_DESCRIPTION_HEADER_MAGIC_HANDLER{ "BVTX" };
	constexpr std::uint32_t CURRENT_VIRTUAL_TEXTURE_DESCRIPTION_VERSION = 2;

	using CurrentVersionedVirtualTextureDescriptionHeader = VersionedVirtualTextureDescriptionHeaderV2;

	struct MergedVirtualTextureDescriptionHeader
	{
		CommonVirtualTextureDescriptionHeader CommonHeader;
		CurrentVersionedVirtualTextureDescriptionHeader VersionedHeader;
	};

	struct SerializedVirtualTexturePageDescription
	{
		// NOTE: ALL virtual texture pages are compressed with zstandard! The compressed size
		// of a given texture page can be calculated as follows:
		//
		// - CompressedSize = NextPage.OffsetFromDescriptionStart - ThisPage.OffsetFromDescriptionStart iff ThisPage is not the last page.
		// - CompressedSize = VirtualTextureDescription.UncompressedSizeInBytes - ThisPage.OffsetFromDescriptionStart iff ThisPage is the last page.

		std::uint64_t OffsetFromFileStart;
		std::uint32_t LogicalMipLevel;
		std::uint32_t PageXCoordinate;
		std::uint32_t PageYCoordinate;
	};

	static constexpr std::uint32_t LOGICAL_PAGE_DIMENSIONS = 128;
}

namespace Brawler
{
	void VirtualTextureMetadata::InitializeFromVirtualTextureFile(const FilePathHash bvtxFileHash)
	{
		const MappedFileView<FileAccessMode::READ_ONLY> virtualTextureFileView{ AssetManagement::BPKArchiveReader::GetInstance().CreateMappedFileViewForAsset(bvtxFileHash) };
		assert(virtualTextureFileView.IsValidView());

		static constexpr std::size_t TOTAL_SERIALIZED_HEADER_SIZE = (sizeof(SerializedStruct<CommonVirtualTextureDescriptionHeader>) + sizeof(SerializedStruct<CurrentVersionedVirtualTextureDescriptionHeader>));

		const std::span<const std::byte> bvtxDataSpan{ virtualTextureFileView.GetMappedData() };
		assert(bvtxDataSpan.size_bytes() >= TOTAL_SERIALIZED_HEADER_SIZE);

		CommonVirtualTextureDescriptionHeader commonHeader{};

		{
			SerializedStruct<CommonVirtualTextureDescriptionHeader> serializedCommonHeader{};
			std::memcpy(&serializedCommonHeader, bvtxDataSpan.data(), sizeof(serializedCommonHeader));

			commonHeader = Brawler::DeserializeData(serializedCommonHeader);
		}

		// Ensure that the file is a valid virtual texture.
		if (commonHeader.Magic != VIRTUAL_TEXTURE_DESCRIPTION_HEADER_MAGIC_HANDLER.GetMagicIntegerValue()) [[unlikely]]
			throw std::runtime_error{ "ERROR: An invalid virtual texture file was detected!" };

		if (commonHeader.Version != CURRENT_VIRTUAL_TEXTURE_DESCRIPTION_VERSION) [[unlikely]]
			throw std::runtime_error{ "ERROR: An outdated virtual texture file was detected!" };

		CurrentVersionedVirtualTextureDescriptionHeader versionedHeader{};

		{
			SerializedStruct<CurrentVersionedVirtualTextureDescriptionHeader> serializedVersionedHeader{};
			std::memcpy(&serializedVersionedHeader, (bvtxDataSpan.data() + sizeof(SerializedStruct<CommonVirtualTextureDescriptionHeader>)), sizeof(SerializedStruct<CurrentVersionedVirtualTextureDescriptionHeader>));

			versionedHeader = Brawler::DeserializeData(serializedVersionedHeader);
		}

		mCopyableFootprintsPageSizeInBytes = versionedHeader.CopyableFootprintsPageSizeInBytes;
		mPageDataFootprint = versionedHeader.PageDataFootprint;
		mLogicalTextureMip0Dimensions = versionedHeader.LogicalTextureMip0Dimensions;
		mLogicalMipLevelCount = versionedHeader.LogicalMipLevelCount;
		mPageMetadataArr.resize(versionedHeader.PageCount);

		assert(Util::Math::IsPowerOfTwo(mLogicalTextureMip0Dimensions) && "ERROR: Virtual texture dimensions are expected to logically be a power of two!");

		// Get a std::span which represents the virtual texture page descriptions contained within the BVTX
		// file. We do this by getting a sub-span of bvtxDataSpan and re-interpreting its data as a span
		// of SerializedStruct<SerializedVirtualTexturePageDescription>.
		const std::span<const std::byte> pageDescriptionByteSpan{ bvtxDataSpan.subspan(TOTAL_SERIALIZED_HEADER_SIZE, (sizeof(SerializedStruct<SerializedVirtualTexturePageDescription>) * versionedHeader.PageCount)) };
		assert(!pageDescriptionByteSpan.empty() || versionedHeader.PageCount == 0);

		const std::span<const SerializedStruct<SerializedVirtualTexturePageDescription>> serializedPageDescriptionSpan{ reinterpret_cast<const SerializedStruct<SerializedVirtualTexturePageDescription>*>(pageDescriptionByteSpan.data()), versionedHeader.PageCount };

		std::vector<SerializedVirtualTexturePageDescription> deserializedPageDescriptionArr{};
		deserializedPageDescriptionArr.reserve(serializedPageDescriptionSpan.size());

		for (const auto& serializedPageDescription : serializedPageDescriptionSpan)
			deserializedPageDescriptionArr.push_back(Brawler::DeserializeData(serializedPageDescription));

		for (const auto i : std::views::iota(0ull, deserializedPageDescriptionArr.size()))
		{
			const SerializedVirtualTexturePageDescription& currPageDescription{ deserializedPageDescriptionArr[i] };
			const std::size_t pageMetadataArrIndex = CalculatePageMetadataArrayIndex(currPageDescription.LogicalMipLevel, currPageDescription.PageXCoordinate, currPageDescription.PageYCoordinate);

			// The file does not explicitly include the compressed size of each virtual texture page, but
			// this data can be inferred based on the offsets to these pages.
			std::size_t compressedPageSize = 0;

			if (i < (serializedPageDescriptionSpan.size() - 1)) [[likely]]
				compressedPageSize = (deserializedPageDescriptionArr[i + 1].OffsetFromFileStart - currPageDescription.OffsetFromFileStart);
			else [[unlikely]]
				compressedPageSize = (bvtxDataSpan.size_bytes() - currPageDescription.OffsetFromFileStart);

			mPageMetadataArr[pageMetadataArrIndex] = VirtualTexturePageMetadata{
				.OffsetFromBVTXFileStartToPageData = currPageDescription.OffsetFromFileStart,
				.CompressedSize = compressedPageSize
			};
		}
	}

	const VirtualTexturePageMetadata& VirtualTextureMetadata::GetPageMetadata(const std::uint32_t mipLevel, const std::uint32_t pageXCoord, const std::uint32_t pageYCoord) const
	{
		return mPageMetadataArr[CalculatePageMetadataArrayIndex(mipLevel, pageXCoord, pageYCoord)];
	}

	std::size_t VirtualTextureMetadata::GetPageCountForMipLevel(const std::uint32_t mipLevel) const
	{
		assert(mipLevel < mLogicalMipLevelCount);

		const std::size_t numPagesPerMip0Row = (mLogicalTextureMip0Dimensions / LOGICAL_PAGE_DIMENSIONS);
		const std::size_t numPagesPerMipLevelRow = (numPagesPerMip0Row >> mipLevel);

		return std::max<std::size_t>(numPagesPerMipLevelRow * numPagesPerMipLevelRow, 1);
	}

	std::uint32_t VirtualTextureMetadata::GetFirstMipLevelInCombinedPage() const
	{
		// The combined page can contain mip levels starting at 64 x 64 and going down to 1 x 1.
		// (NOTE: These are logical dimensions, and do not account for padding for the purposes
		// of filtering.)
		//
		// This represents a total of seven potential mip levels, which is 
		// log2(LOGICAL_PAGE_DIMENSIONS).
		static constexpr std::int32_t MAX_MIP_LEVEL_COUNT_IN_COMBINED_PAGE = std::countr_zero(LOGICAL_PAGE_DIMENSIONS);

		return static_cast<std::uint32_t>(std::max<std::int32_t>(static_cast<std::int32_t>(mLogicalMipLevelCount) - MAX_MIP_LEVEL_COUNT_IN_COMBINED_PAGE, 0));
	}

	DXGI_FORMAT VirtualTextureMetadata::GetTextureFormat() const
	{
		return mPageDataFootprint.Format;
	}

	const D3D12_SUBRESOURCE_FOOTPRINT& VirtualTextureMetadata::GetPageDataFootprint() const
	{
		return mPageDataFootprint;
	}

	std::uint32_t VirtualTextureMetadata::GetLogicalMipLevel0Dimensions() const
	{
		return mLogicalTextureMip0Dimensions;
	}

	std::uint32_t VirtualTextureMetadata::GetLogicalMipLevelCount() const
	{
		return mLogicalMipLevelCount;
	}

	std::size_t VirtualTextureMetadata::GetCopyableFootprintsPageSize() const
	{
		return mCopyableFootprintsPageSizeInBytes;
	}

	std::size_t VirtualTextureMetadata::CalculatePageMetadataArrayIndex(const std::uint32_t mipLevel, const std::uint32_t pageXCoord, const std::uint32_t pageYCoord) const
	{
		assert(mipLevel < mLogicalMipLevelCount);

		const std::uint32_t clampedMipLevel = std::clamp<std::uint32_t>(mipLevel, 0, GetFirstMipLevelInCombinedPage());
		const std::uint32_t numPagesPerMip0Dimension = (mLogicalTextureMip0Dimensions / LOGICAL_PAGE_DIMENSIONS);
		const std::uint32_t numPagesPerMipLevelDimension = (numPagesPerMip0Dimension >> clampedMipLevel);

		assert(pageXCoord < numPagesPerMipLevelDimension && pageYCoord < numPagesPerMipLevelDimension);
		
		const std::uint32_t flattenedPageIndex = ((pageYCoord * numPagesPerMipLevelDimension) + pageXCoord);

		// The number of pages consumed by a given mip level can be represented as a geometric
		// series for the first N mip levels, where N is the number of mip levels which are not
		// placed into a combined page. Realizing this, we can calculate the number of pages used
		// by all previous mip levels in constant time by finding the partial sum of the geometric series.
		//
		// The geometric series itself is a_n = a_0 * 4^n for all n >= 0, where a_0 = the number of
		// pages used by mip level 0 and n is the mip level.

		const std::size_t numPagesUsedByPreviousMipLevels = static_cast<std::size_t>(static_cast<std::int32_t>(numPagesPerMip0Dimension << 1) * (1 - (1 << (2 * mipLevel))) / -3);
		return (numPagesUsedByPreviousMipLevels + flattenedPageIndex);
	}
}