module;
#include <source_location>
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

export namespace Util
{
	namespace D3D12
	{
		/// <summary>
		/// Determines the number of bits per pixel for a given texture with the specified
		/// DXGI_FORMAT. If an error occurs, then the function returns 0.
		/// 
		/// (The implementation for this was ripped wholesale from DirectXTex. I just copied the
		/// entire function because DirectX::BitsPerPixel() in DirectXTex is not constexpr,
		/// although it very well could be.)
		/// </summary>
		/// <param name="format">
		/// - The DXGI_FORMAT for which the number of bits per pixel is to be returned.
		/// </param>
		/// <returns>
		/// If successful, the function returns the number of bits per pixel for a given texture
		/// with the specified DXGI_FORMAT. If an error occurs, then the function returns 0.
		/// </returns>
		constexpr std::size_t GetBitsPerPixelForFormat(const DXGI_FORMAT format)
		{
			switch (format)
			{
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT:
				return 128;

			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32B32_SINT:
				return 96;

			case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SINT:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R32G32_SINT:
			case DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_Y416:
			case DXGI_FORMAT::DXGI_FORMAT_Y210:
			case DXGI_FORMAT::DXGI_FORMAT_Y216:
				return 64;

			case DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R11G11B10_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SINT:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16_SNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R16G16_SINT:
			case DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_R32_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R32_SINT:
			case DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8_B8G8_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_G8R8_G8B8_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			case DXGI_FORMAT::DXGI_FORMAT_AYUV:
			case DXGI_FORMAT::DXGI_FORMAT_Y410:
			case DXGI_FORMAT::DXGI_FORMAT_YUY2:
				return 32;

			case DXGI_FORMAT::DXGI_FORMAT_P010:
			case DXGI_FORMAT::DXGI_FORMAT_P016:
				return 24;

			case DXGI_FORMAT::DXGI_FORMAT_R8G8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8_SNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R8G8_SINT:
			case DXGI_FORMAT::DXGI_FORMAT_R16_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT:
			case DXGI_FORMAT::DXGI_FORMAT_D16_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R16_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R16_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R16_SNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R16_SINT:
			case DXGI_FORMAT::DXGI_FORMAT_B5G6R5_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_B5G5R5A1_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_A8P8:
			case DXGI_FORMAT::DXGI_FORMAT_B4G4R4A4_UNORM:
				return 16;

			case DXGI_FORMAT::DXGI_FORMAT_NV12:
			case DXGI_FORMAT::DXGI_FORMAT_420_OPAQUE:
			case DXGI_FORMAT::DXGI_FORMAT_NV11:
				return 12;

			case DXGI_FORMAT::DXGI_FORMAT_R8_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_R8_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R8_UINT:
			case DXGI_FORMAT::DXGI_FORMAT_R8_SNORM:
			case DXGI_FORMAT::DXGI_FORMAT_R8_SINT:
			case DXGI_FORMAT::DXGI_FORMAT_A8_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_BC2_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB:
			case DXGI_FORMAT::DXGI_FORMAT_BC3_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB:
			case DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM:
			case DXGI_FORMAT::DXGI_FORMAT_BC6H_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC6H_UF16:
			case DXGI_FORMAT::DXGI_FORMAT_BC6H_SF16:
			case DXGI_FORMAT::DXGI_FORMAT_BC7_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB:
			case DXGI_FORMAT::DXGI_FORMAT_AI44:
			case DXGI_FORMAT::DXGI_FORMAT_IA44:
			case DXGI_FORMAT::DXGI_FORMAT_P8:
				return 8;

			case DXGI_FORMAT::DXGI_FORMAT_R1_UNORM:
				return 1;

			case DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB:
			case DXGI_FORMAT::DXGI_FORMAT_BC4_TYPELESS:
			case DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM:
			case DXGI_FORMAT::DXGI_FORMAT_BC4_SNORM:
				return 4;

			default:
				return 0;
			}
		}
	}
}

export namespace Util
{
	namespace D3D12
	{
		struct SubResourceDecompositionResults
		{
			std::uint32_t MipSlice;
			std::uint32_t ArraySlice;
			std::uint32_t PlaneSlice;
		};

		constexpr SubResourceDecompositionResults DecomposeSubResource(const std::uint32_t subResourceIndex, const std::uint32_t numMipLevels, const std::uint32_t arraySize)
		{
			// The implementation for this is basically a carbon copy of what can be found on the MSDN at
			// https://docs.microsoft.com/en-us/windows/win32/direct3d12/d3d12decomposesubresource. I just
			// re-implemented it as a constexpr function, since it isn't marked as such in the D3D12 header
			// file.

			return SubResourceDecompositionResults{
				.MipSlice = (subResourceIndex % numMipLevels),
				.ArraySlice = ((subResourceIndex / numMipLevels) % arraySize),
				.PlaneSlice = (subResourceIndex / (numMipLevels * arraySize))
			};
		}
	}
}

export namespace Util
{
	namespace D3D12
	{
		struct CopyableFootprintsParams
		{
			const Brawler::D3D12_RESOURCE_DESC ResourceDesc;
			std::uint32_t FirstSubResource;
			std::uint32_t NumSubResources;
			std::uint64_t BaseOffset;
		};

		struct CopyableFootprint
		{
			/// <summary>
			/// Provides additional information regarding the sub-allocation within an
			/// upload buffer which must be designated for this sub-resource. Note that
			/// Layout.Footprint.RowPitch *IS* aligned to 
			/// D3D12_TEXTURE_DATA_PITCH_ALIGNMENT. For more details, refer to
			/// https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_placed_subresource_footprint.
			/// </summary>
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layout;

			/// <summary>
			/// The number of rows consumed by this sub-resource. This should be the same
			/// as Layout.Footprint.Height.
			/// </summary>
			std::uint32_t NumRows;

			/// <summary>
			/// The *UNPADDED* size, in bytes, of a row of texture data for this
			/// sub-resource. Unlike Layout.Footprint.RowPitch, this value *MIGHT NOT* be
			/// aligned to D3D12_TEXTURE_DATA_PITCH_ALIGNMENT.
			/// </summary>
			std::uint64_t RowSizeInBytes;
		};

		struct CopyableFootprintsResults
		{
			/// <summary>
			/// A std::vector&lt;CopyableFootprint&gt; instance which describes, for each
			/// requested sub-resource, the layout, number of rows, and *UNPADDED* row size
			/// of said sub-resource. More details can be found in the documentation of
			/// CopyableFootprint, as well as in the MSDN.
			/// </summary>
			std::vector<CopyableFootprint> FootprintsArr;

			/// <summary>
			/// Returns the required upload buffer size for copying all of the sub-resources
			/// requested. This value does account for any padding required by the D3D12 API,
			/// both for the required alignment of texture data within a buffer and the required
			/// alignment of texture row data.
			/// </summary>
			std::uint64_t TotalBytes;

			constexpr CopyableFootprintsResults() :
				FootprintsArr(),
				TotalBytes(0)
			{}

			constexpr CopyableFootprintsResults(const CopyableFootprintsResults& rhs) = default;
			constexpr CopyableFootprintsResults& operator=(const CopyableFootprintsResults& rhs) = default;

			constexpr CopyableFootprintsResults(CopyableFootprintsResults&& rhs) noexcept = default;
			constexpr CopyableFootprintsResults& operator=(CopyableFootprintsResults&& rhs) noexcept = default;
		};

		/// <summary>
		/// Attempts to evaluate the result of ID3D12Device::GetCopyableFootprints1() in a
		/// constant-evaluated context. Since the return value of that function is device-agnostic,
		/// it should theoretically be a good candidate for constexpr. However, few, if any, functions
		/// in the D3D12 API are actually labeled constexpr. (I guess they wanted to remain compatible
		/// with C++11, but couldn't they just use pre-processor macros like the MSVC STL?)
		/// 
		/// Testing should be done to validate the results of this function. The implementation of
		/// ID3D12Device::GetCopyableFootprints1() is not open source, but the MSDN states that all of
		/// the details needed to implement the function are exposed to application developers. The
		/// returned result should work for most trivial (i.e., non-planar) DXGI_FORMAT values, but again,
		/// this should be verified with ground truth results from the D3D12 API.
		/// 
		/// The function is consteval, rather than constexpr, for two reasons:
		///	  1. Unlike ID3D12Device::GetCopyableFootprints1() which calculates only the return values
		///      which you ask for, this function calculates and returns every value. This is fine at
		///      compile time, but is admittedly less efficient at run-time.
		/// 
		///	  2. ID3D12Device::GetCopyableFootprints1() is guaranteed to always return correct results.
		///      This function should work for most trivial (i.e., non-planar) DXGI_FORMAT values, but
		///      the returned values should be verified with ground truth results from the D3D12 API.
		///      So, if you have to get this information at run-time for some reason, then you should
		///      always prefer ID3D12Device::GetCopyableFootprints1().
		/// </summary>
		/// <param name="params">
		/// - A const CopyableFootprintsParams&amp; which describes the sub-resource(s) whose copyable footprint(s)
		///   are to be returned. Rather than having a function with eight parameters, this function wraps
		///   the input parameters into a struct and returns the output values in another struct.
		/// </param>
		/// <returns>
		/// The function returns a CopyableFootprintsResults instance which describes the total size
		/// of the upload buffer required to upload all sub-resources requested, as well as a CopyableFootprint
		/// instance for each sub-resource requested.
		/// </returns>
		consteval CopyableFootprintsResults GetCopyableFootprints(const CopyableFootprintsParams& params)
		{
			// The assert() macro is causing the MSVC to spit out false errors (again).
			constexpr auto SAFE_ASSERT = [] (const bool expression, const std::source_location srcLocation = std::source_location::current())
			{
				(void)(
					(!!(expression)) ||
					(_wassert(L"ERROR: An assertion failed!", L"D3D12UtilFormats.ixx", srcLocation.line()), 0)
					);
			};
			
			SAFE_ASSERT(params.FirstSubResource < params.NumSubResources);
			SAFE_ASSERT(params.ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER && params.ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_UNKNOWN);

			SAFE_ASSERT(params.ResourceDesc.Format != DXGI_FORMAT::DXGI_FORMAT_UNKNOWN);
			const std::size_t numBitsPerPixel = GetBitsPerPixelForFormat(params.ResourceDesc.Format);

			const std::uint32_t numMipLevels = params.ResourceDesc.MipLevels;
			const std::uint32_t numArrayElements = (params.ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : params.ResourceDesc.DepthOrArraySize);

			constexpr auto ALIGN_TO_POWER_OF_TWO = []<std::size_t Alignment>(const std::uint64_t value)
			{
				return ((value + (Alignment - 1)) & ~(Alignment - 1));
			};

			CopyableFootprintsResults results{};
			std::uint64_t currUploadOffset = ALIGN_TO_POWER_OF_TWO.operator()<D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT>(params.BaseOffset);

			for (std::uint32_t i = 0; i < params.NumSubResources; ++i)
			{
				CopyableFootprint currFootprint{};
				const std::uint32_t currSubResource = (params.FirstSubResource + i);

				const SubResourceDecompositionResults decomposedResults{ DecomposeSubResource(currSubResource, numMipLevels, numArrayElements) };

				// TODO: Does this still work for multi-planar formats?
				const std::uint64_t subResourceWidth = (params.ResourceDesc.Width >> decomposedResults.MipSlice);
				const std::uint32_t subResourceHeight = (params.ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D ? 1 : (params.ResourceDesc.Height >> decomposedResults.MipSlice));
				const std::uint32_t subResourceDepth = (params.ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : (params.ResourceDesc.DepthOrArraySize >> decomposedResults.MipSlice));

				const std::size_t subResourceBitsPerRow = (subResourceWidth * numBitsPerPixel);
				const std::size_t subResourceUnalignedBytesPerRow = ((subResourceBitsPerRow / 8) + std::min<std::size_t>(subResourceBitsPerRow % 8, 1));

				currFootprint.Layout = D3D12_PLACED_SUBRESOURCE_FOOTPRINT{
					.Offset = currUploadOffset,
					.Footprint{
						.Format = params.ResourceDesc.Format,
						.Width = static_cast<std::uint32_t>(subResourceWidth),
						.Height = subResourceHeight,
						.Depth = subResourceDepth,
						.RowPitch = static_cast<std::uint32_t>(ALIGN_TO_POWER_OF_TWO.operator()<D3D12_TEXTURE_DATA_PITCH_ALIGNMENT>(subResourceUnalignedBytesPerRow))
					}
				};

				currFootprint.NumRows = subResourceHeight;
				currFootprint.RowSizeInBytes = subResourceUnalignedBytesPerRow;

				const std::uint64_t totalSubResourceSize = (static_cast<std::uint64_t>(currFootprint.Layout.Footprint.RowPitch) * static_cast<std::uint64_t>(subResourceHeight) * currFootprint.Layout.Footprint.Depth);
				results.TotalBytes += totalSubResourceSize;

				results.FootprintsArr.push_back(std::move(currFootprint));

				// Adjust the offset from the start of the buffer for the next sub-resource, but only
				// if there are more sub-resources which need to be processed.
				if (i != (params.NumSubResources - 1)) [[likely]]
				{
					const std::uint64_t unalignedNewUploadOffset = (currUploadOffset + totalSubResourceSize);
					currUploadOffset = ALIGN_TO_POWER_OF_TWO.operator() < D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT > (unalignedNewUploadOffset);

					// If aligning unalignedNewUploadOffset to D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT required padding
					// out the buffer, then the returned size needs to reflect that.
					results.TotalBytes += (currUploadOffset - unalignedNewUploadOffset);
				}
			}

			return results;
		}
	}
}