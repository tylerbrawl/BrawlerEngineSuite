module;
#include <vector>
#include <DxDef.h>

export module Brawler.VirtualTextureMetadata;
import Brawler.FilePathHash;

export namespace Brawler
{
	struct VirtualTexturePageMetadata
	{
		std::uint64_t OffsetFromBVTXFileStartToPageData;
		std::uint64_t CompressedSize;
	};
}

export namespace Brawler
{
	class VirtualTextureMetadata
	{
	public:
		VirtualTextureMetadata() = default;

		VirtualTextureMetadata(const VirtualTextureMetadata& rhs) = delete;
		VirtualTextureMetadata& operator=(const VirtualTextureMetadata& rhs) = delete;

		VirtualTextureMetadata(VirtualTextureMetadata&& rhs) noexcept = default;
		VirtualTextureMetadata& operator=(VirtualTextureMetadata&& rhs) noexcept = default;

		void InitializeFromVirtualTextureFile(const FilePathHash bvtxFileHash);

		const VirtualTexturePageMetadata& GetPageMetadata(const std::uint32_t mipLevel, const std::uint32_t pageXCoord, const std::uint32_t pageYCoord) const;
		std::size_t GetPageCountForMipLevel(const std::uint32_t mipLevel) const;
		std::uint32_t GetFirstMipLevelInCombinedPage() const;

		DXGI_FORMAT GetTextureFormat() const;
		std::uint32_t GetLogicalMipLevel0Dimensions() const;
		std::uint32_t GetLogicalMipLevelCount() const;

		std::size_t GetCopyableFootprintsPageSize() const;

	private:
		std::size_t CalculatePageMetadataArrayIndex(const std::uint32_t mipLevel, const std::uint32_t pageXCoord, const std::uint32_t pageYCoord) const;

	private:
		std::uint64_t mCopyableFootprintsPageSizeInBytes;
		DXGI_FORMAT mTextureFormat;
		std::uint32_t mLogicalTextureMip0Dimensions;
		std::uint32_t mLogicalMipLevelCount;
		std::vector<VirtualTexturePageMetadata> mPageMetadataArr;
	};
}