module;
#include <cstdint>
#include <cstddef>
#include <DirectXMath/DirectXMath.h>

export module Brawler.VirtualTextureConstants;
import Brawler.VirtualTexturePageFilterMode;

export namespace Brawler
{
	namespace VirtualTextures
	{
		constexpr DirectX::XMUINT2 USEFUL_VIRTUAL_TEXTURE_PAGE_DIMENSIONS{ 128, 128 };
	}
}

namespace Brawler
{
	namespace VirtualTextures
	{
		template <VirtualTexturePageFilterMode FilterMode>
		struct FilterModeInfo
		{
			static_assert(sizeof(FilterMode) != sizeof(FilterMode), "ERROR: An explicit template specialization of Brawler::FilterModeInfo was never provided for a given Brawler::VirtualTexturePageFilterMode instance! (See VirtualTextureConstants.ixx.)");
		};

		template <std::uint32_t BorderDimensions>
		struct FilterModeInfoInstantiation
		{
			static constexpr DirectX::XMUINT2 BORDER_DIMENSIONS{ BorderDimensions, BorderDimensions };
			static constexpr DirectX::XMUINT2 PAGE_DIMENSIONS{ (USEFUL_VIRTUAL_TEXTURE_PAGE_DIMENSIONS.x + (BORDER_DIMENSIONS.x * 2)), (USEFUL_VIRTUAL_TEXTURE_PAGE_DIMENSIONS.y + (BORDER_DIMENSIONS.y * 2)) };
		};

		template <>
		struct FilterModeInfo<VirtualTexturePageFilterMode::POINT_FILTER> : public FilterModeInfoInstantiation<0>
		{};

		template <>
		struct FilterModeInfo<VirtualTexturePageFilterMode::BILINEAR_FILTER> : public FilterModeInfoInstantiation<1>
		{};

		template <>
		struct FilterModeInfo<VirtualTexturePageFilterMode::TRILINEAR_FILTER> : public FilterModeInfoInstantiation<2>
		{};

		template <>
		struct FilterModeInfo<VirtualTexturePageFilterMode::ANISOTROPIC_8X_FILTER> : public FilterModeInfoInstantiation<3>
		{};
	}
}

export namespace Brawler
{
	namespace VirtualTextures
	{
		template <VirtualTexturePageFilterMode FilterMode>
		consteval DirectX::XMUINT2 GetPageFilterBorderDimensions()
		{
			return FilterModeInfo<FilterMode>::BORDER_DIMENSIONS;
		}

		template <VirtualTexturePageFilterMode FilterMode>
		consteval DirectX::XMUINT2 GetTotalPageDimensions()
		{
			return FilterModeInfo<FilterMode>::PAGE_DIMENSIONS;
		}
	}
}