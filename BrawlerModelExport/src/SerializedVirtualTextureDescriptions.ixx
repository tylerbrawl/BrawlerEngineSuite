module;
#include <cstdint>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

export module Brawler.SerializedVirtualTextureDescriptions;
import Brawler.FileMagicHandler;

export namespace Brawler
{
#pragma pack(push)
#pragma pack(1)
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

	constexpr FileMagicHandler VIRTUAL_TEXTURE_DESCRIPTION_HEADER_MAGIC_HANDLER{ "BVTX" };
	constexpr std::uint32_t CURRENT_VIRTUAL_TEXTURE_DESCRIPTION_VERSION = 1;

	using CurrentVersionedVirtualTextureDescriptionHeader = VersionedVirtualTextureDescriptionHeaderV1;

	struct MergedVirtualTextureDescriptionHeader
	{
		CommonVirtualTextureDescriptionHeader CommonHeader;
		CurrentVersionedVirtualTextureDescriptionHeader VersionedHeader;
	};

	struct SerializedVirtualTexturePageDescription
	{
		std::uint64_t LogicalMipLevel;
		DirectX::XMUINT2 PageCoordinates;
		std::uint64_t PageTextureDataFilePathHash;
	};
#pragma pack(pop)
}

export namespace Brawler
{
	consteval CommonVirtualTextureDescriptionHeader CreateCommonVirtualTextureDescriptionHeader()
	{
		return CommonVirtualTextureDescriptionHeader{
			.Magic = VIRTUAL_TEXTURE_DESCRIPTION_HEADER_MAGIC_HANDLER.GetMagicIntegerValue(),
			.Version = CURRENT_VIRTUAL_TEXTURE_DESCRIPTION_VERSION
		};
	}
}