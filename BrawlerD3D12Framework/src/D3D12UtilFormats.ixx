module;
#include "DxDef.h"

export module Util.D3D12:Formats;

namespace Util
{
	namespace D3D12
	{
		// When it comes to D3D12 documentation, you have to search the deep, dark bowels of Hell to even
		// get a hint. Anyways, the "conclusive" list of D3D12 type families can be found as a comment in 
		// this seemingly unrelated gpuweb issue:
		//
		// https://github.com/gpuweb/gpuweb/issues/168#issuecomment-461992453
		//
		// I guess the part of the MSDN which said that casting could be done from a typeless format to
		// any typed format so long as they share the same number of components and bits per component
		// was wrong.

		enum class TypeFamily
		{
			FAMILY_R32G32B32A32,
			FAMILY_R32G32B32,
			FAMILY_R16G16B16A16,
			FAMILY_R32G32,
			FAMILY_R32G8X24,
			FAMILY_R10G10B10A2,
			FAMILY_R8G8B8A8,
			FAMILY_R16G16,
			FAMILY_R32,
			FAMILY_R24G8,
			FAMILY_R8G8,
			FAMILY_R16,
			FAMILY_R8,
			FAMILY_BC1,
			FAMILY_BC2,
			FAMILY_BC3,
			FAMILY_BC4,
			FAMILY_BC5,
			FAMILY_B8G8R8A8,
			FAMILY_B8G8R8X8,
			FAMILY_BC6H,
			FAMILY_BC7,

			FAMILY_UNKNOWN
		};

		// Also of note in that thread of comments is the clarification which Rafael Cintron provides
		// for casting RTVs, DSVs, SRVs, and UAVs. Specifically, although it isn't mentioned in the MSDN,
		// the rules for casting resources for RTVs and DSVs are the same as those for SRVs: Casting is
		// only allowed from a TYPELESS format to one of the formats in its respective type family.
		//
		// For UAVs, the rules are almost the same, except for the fact that there is a special exception 
		// regarding R32 types. Specifically, it is valid to create a UAV of format R32_UINT, R32_SINT, or 
		// R32_FLOAT from a resource which was initially created with any of the following formats:
		//
		//   - DXGI_FORMAT_R10G10B10A2_TYPELESS
		//   - DXGI_FORMAT_R8G8B8A8_TYPELESS
		//   - DXGI_FORMAT_B8G8R8A8_TYPELESS
		//   - DXGI_FORMAT_B8G8R8X8_TYPELESS
		//   - DXGI_FORMAT_R16G16_TYPELESS
		//   - DXGI_FORMAT_R32_TYPELESS
		//
		// Of course, for any other UAV format, you need to make sure that the device supports typed UAV
		// loads by calling ID3D12Device::CheckFeatureSupport().

		constexpr TypeFamily GetTypeFamilyForDXGIFormat(const DXGI_FORMAT format)
		{
			switch (format)
			{
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT:
				return TypeFamily::FAMILY_R32G32B32A32;

			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_SINT:
				return TypeFamily::FAMILY_R32G32B32;

			case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SINT:
				return TypeFamily::FAMILY_R16G16B16A16;

			case DXGI_FORMAT::DXGI_FORMAT_R32G32_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32_SINT:
				return TypeFamily::FAMILY_R32G32;

			case DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
				return TypeFamily::FAMILY_R32G8X24;

			case DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
				return TypeFamily::FAMILY_R10G10B10A2;

			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SINT:
				return TypeFamily::FAMILY_R8G8B8A8;

			case DXGI_FORMAT::DXGI_FORMAT_R16G16_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16_SNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16_SINT:
				return TypeFamily::FAMILY_R16G16;

			case DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R32_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R32_SINT:
				return TypeFamily::FAMILY_R32;

			case DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_X24_TYPELESS_G8_UINT:
				return TypeFamily::FAMILY_R24G8;

			case DXGI_FORMAT::DXGI_FORMAT_R8G8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8_SNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8_SINT:
				return TypeFamily::FAMILY_R8G8;

			case DXGI_FORMAT::DXGI_FORMAT_R16_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_D16_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R16_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R16_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R16_SNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R16_SINT:
				return TypeFamily::FAMILY_R16;

			case DXGI_FORMAT::DXGI_FORMAT_R8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R8_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R8_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R8_SNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R8_SINT:
				return TypeFamily::FAMILY_R8;

			case DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB:
				return TypeFamily::FAMILY_BC1;

			case DXGI_FORMAT::DXGI_FORMAT_BC2_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB:
				return TypeFamily::FAMILY_BC2;

			case DXGI_FORMAT::DXGI_FORMAT_BC3_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB:
				return TypeFamily::FAMILY_BC3;

			case DXGI_FORMAT::DXGI_FORMAT_BC4_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_BC4_SNORM:
				return TypeFamily::FAMILY_BC4;

			case DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM:
				return TypeFamily::FAMILY_BC5;

			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
				return TypeFamily::FAMILY_B8G8R8A8;

			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
				return TypeFamily::FAMILY_B8G8R8X8;

			case DXGI_FORMAT::DXGI_FORMAT_BC6H_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC6H_UF16:
			case DXGI_FORMAT::DXGI_FORMAT_BC6H_SF16:
				return TypeFamily::FAMILY_BC6H;

			case DXGI_FORMAT::DXGI_FORMAT_BC7_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB:
				return TypeFamily::FAMILY_BC7;

			default:
				return TypeFamily::FAMILY_UNKNOWN;
			}
		}

		constexpr bool IsFormatCompletelyTypeless(const DXGI_FORMAT format)
		{
			switch (format)
			{
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R16_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC2_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC3_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC4_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC6H_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC7_TYPELESS:
				return true;

			default:
				return false;
			}
		}
	}
}

export namespace Util
{
	namespace D3D12
	{
		constexpr bool IsSRVRTVDSVResourceCastLegal(const DXGI_FORMAT fromFormat, const DXGI_FORMAT toFormat)
		{
			// If the two formats are the same, then the "cast" is always legal.
			if (fromFormat == toFormat)
				return true;
			
			// As per the rules described above, for a resource cast to be legal for
			// the sake of creating SRVs, RTVs, or DSVs, FromFormat must be a typeless
			// format. In addition, FromFormat and ToFormat must be in the same type
			// family.

			if (!IsFormatCompletelyTypeless(fromFormat))
				return false;

			const TypeFamily fromFormatFamily = GetTypeFamilyForDXGIFormat(fromFormat);
			const TypeFamily toFormatFamily = GetTypeFamilyForDXGIFormat(toFormat);

			if (fromFormatFamily == TypeFamily::FAMILY_UNKNOWN || toFormatFamily == TypeFamily::FAMILY_UNKNOWN)
				return false;

			return (fromFormatFamily == toFormatFamily);
		}

		constexpr bool IsUAVResourceCastLegal(const DXGI_FORMAT fromFormat, const DXGI_FORMAT toFormat)
		{
			// As per the rules above, UAV casting rules are similar to those for SRVs,
			// RTVs, and DSVs. However, R32 types have special exceptions.

			if (IsSRVRTVDSVResourceCastLegal(fromFormat, toFormat))
				return true;

			if (toFormat != DXGI_FORMAT::DXGI_FORMAT_R32_UINT && toFormat != DXGI_FORMAT::DXGI_FORMAT_R32_SINT &&
				toFormat != DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT)
				return false;

			switch (fromFormat)
			{
			case DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS:
				return true;

			default:
				return false;
			}
		}
	}
}