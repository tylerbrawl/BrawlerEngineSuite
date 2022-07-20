module;
#include <DxDef.h>

export module Brawler.GlobalTextureFormatInfo;
import Brawler.D3D12.Texture2DBuilders;

export namespace Brawler
{
	template <DXGI_FORMAT Format>
	struct GlobalTextureFormatInfo
	{
		static_assert(sizeof(Format) != sizeof(Format), "ERROR: An explicit template specialization of GlobalTextureFormatInfo was never provided for a given DXGI_FORMAT value! (See GlobalTexture.ixx.)");
	};

	template <>
	struct GlobalTextureFormatInfo<DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB>
	{
		static constexpr std::size_t GLOBAL_TEXTURE_COUNT = 1;
		static constexpr std::uint32_t PADDED_PAGE_DIMENSIONS = 136;
	};
}

export namespace Brawler
{
	template <DXGI_FORMAT Format>
	consteval D3D12::Texture2DBuilder CreateGlobalTextureBuilder()
	{
		D3D12::Texture2DBuilder globalTextureBuilder{};
		globalTextureBuilder.DenyUnorderedAccessViews();
		globalTextureBuilder.SetInitialResourceState(D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
		globalTextureBuilder.SetMipLevelCount(1);
		globalTextureBuilder.SetTextureFormat(Format);

		// Create the largest possible two-dimensional texture which can hold the pages for this
		// particular format. Pages with padding will no longer have dimensions which are powers
		// of two; thus, we should adjust our texture dimensions so that we can only include
		// entire pages.

		// This value was found from the MSDN at 
		// https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-devices-downlevel-intro#feature-support-for-feature-levels-12_2-through-9_3.
		constexpr std::size_t MAX_SUPPORTED_TEXTURE_DIMENSIONS = 16384;

		// Use integer division to conveniently get the number of whole pages which can be represented
		// in a row/column of MAX_SUPPORTED_TEXTURE_DIMENSIONS texels.
		constexpr std::size_t MAX_WHOLE_PAGE_COUNT_PER_DIMENSION = (MAX_SUPPORTED_TEXTURE_DIMENSIONS / GlobalTextureFormatInfo<Format>::PADDED_PAGE_DIMENSIONS);

		// Multiply that value by PADDED_PAGE_DIMENSIONS to get the appropriate texture dimensions.
		constexpr std::size_t GLOBAL_TEXTURE_DIMENSIONS = (MAX_WHOLE_PAGE_COUNT_PER_DIMENSION * GlobalTextureFormatInfo<Format>::PADDED_PAGE_DIMENSIONS);

		globalTextureBuilder.SetTextureDimensions(GLOBAL_TEXTURE_DIMENSIONS, GLOBAL_TEXTURE_DIMENSIONS);

		return globalTextureBuilder;
	}
}