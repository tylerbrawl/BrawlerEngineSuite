#include "VirtualTextureConstants.hlsli"
#include "ColorSpaceUtil.hlsli"

static const uint POINT_FILTER_TILING_MODE = 0;
static const uint BILINEAR_FILTER_TILING_MODE = 1;
static const uint TRILINEAR_FILTER_TILING_MODE = 2;
static const uint ANISOTROPIC_8X_FILTER_TILING_MODE = 3;

template <uint TilingModeNum>
struct TilingTypeInfo
{};

template <>
struct TilingTypeInfo<POINT_FILTER_TILING_MODE>
{
	static const uint2 BORDER_DIMENSIONS;
	static const uint2 PAGE_DIMENSIONS;
};

static const uint2 TilingTypeInfo<POINT_FILTER_TILING_MODE>::BORDER_DIMENSIONS = uint2(0, 0);
static const uint2 TilingTypeInfo<POINT_FILTER_TILING_MODE>::PAGE_DIMENSIONS = VirtualTextures::USEFUL_PAGE_DIMENSIONS + (TilingTypeInfo<POINT_FILTER_TILING_MODE>::BORDER_DIMENSIONS * 2);

template <>
struct TilingTypeInfo<BILINEAR_FILTER_TILING_MODE>
{
	static const uint2 BORDER_DIMENSIONS;
	static const uint2 PAGE_DIMENSIONS;
};

static const uint2 TilingTypeInfo<BILINEAR_FILTER_TILING_MODE>::BORDER_DIMENSIONS = uint2(1, 1);
static const uint2 TilingTypeInfo<BILINEAR_FILTER_TILING_MODE>::PAGE_DIMENSIONS = VirtualTextures::USEFUL_PAGE_DIMENSIONS + (TilingTypeInfo<BILINEAR_FILTER_TILING_MODE>::BORDER_DIMENSIONS * 2);

template <>
struct TilingTypeInfo<TRILINEAR_FILTER_TILING_MODE>
{
	static const uint2 BORDER_DIMENSIONS;
	static const uint2 PAGE_DIMENSIONS;
};

static const uint2 TilingTypeInfo<TRILINEAR_FILTER_TILING_MODE>::BORDER_DIMENSIONS = uint2(2, 2);
static const uint2 TilingTypeInfo<TRILINEAR_FILTER_TILING_MODE>::PAGE_DIMENSIONS = VirtualTextures::USEFUL_PAGE_DIMENSIONS + (TilingTypeInfo<TRILINEAR_FILTER_TILING_MODE>::BORDER_DIMENSIONS * 2);

template <>
struct TilingTypeInfo<ANISOTROPIC_8X_FILTER_TILING_MODE>
{
	static const uint2 BORDER_DIMENSIONS;
	static const uint2 PAGE_DIMENSIONS;
};

static const uint2 TilingTypeInfo<ANISOTROPIC_8X_FILTER_TILING_MODE>::BORDER_DIMENSIONS = uint2(4, 4);
static const uint2 TilingTypeInfo<ANISOTROPIC_8X_FILTER_TILING_MODE>::PAGE_DIMENSIONS = VirtualTextures::USEFUL_PAGE_DIMENSIONS + (TilingTypeInfo<ANISOTROPIC_8X_FILTER_TILING_MODE>::BORDER_DIMENSIONS * 2);

#if defined(__TILING_MODE_POINT_FILTER__)
typedef TilingTypeInfo<POINT_FILTER_TILING_MODE> CurrentTilingTypeInfo;
#elif defined(__TILING_MODE_BILINEAR_FILTER__)
typedef TilingTypeInfo<BILINEAR_FILTER_TILING_MODE> CurrentTilingTypeInfo;
#elif defined(__TILING_MODE_TRILINEAR_FILTER__)
typedef TilingTypeInfo<TRILINEAR_FILTER_TILING_MODE> CurrentTilingTypeInfo;
#elif defined(__TILING_MODE_ANISOTROPIC_8X_FILTER__)
typedef TilingTypeInfo<ANISOTROPIC_8X_FILTER_TILING_MODE> CurrentTilingTypeInfo;
#else
#error "ERROR: A tiling mode *MUST* be specified as a preprocessor definition! (Valid modes are __TILING_MODE_POINT_FILTER__, __TILING_MODE_BILINEAR_FILTER__, __TILING_MODE_TRILINEAR_FILTER__, and __TILING_MODE_ANISOTROPIC_8X_FILTER__.)"
#endif

struct ConstantsInfo
{
	float2 InverseInputDimensions;
	uint InputMipLevel;
};

Texture2D<float4> InputTexture : register(t0, space0);
ConstantBuffer<ConstantsInfo> TilingConstants : register(b0, space0);

RWTexture2D<float4> OutputTexture : register(u0, space0);

SamplerState PointClampSampler : register(s0, space0);

// ===========================================================================================================
// COMPUTE SHADER CONTRACT
//
// Thread Group Dimensions: 8 x 8 x 1
//
// Description: Launch one thread for each texel in OutputTexture.
// ===========================================================================================================
[numthreads(8, 8, 1)]
void main(in const uint2 DTid : SV_DispatchThreadID)
{
	const uint2 dimensionsWithinPage = DTid % CurrentTilingTypeInfo::PAGE_DIMENSIONS;
	const uint2 pageIndex = DTid / CurrentTilingTypeInfo::PAGE_DIMENSIONS;
	
	// For virtual texturing, if we want to support hardware texture filtering (and to not support it would
	// be rather stupid), we need to add a border of redundant texels around each page. The contents of the
	// border texels should be copied from the texels adjacent to the useful page texels in the source
	// texture.
	//
	// Assuming that each lane represents a texel in OutputTexture, we can actually calculate the texel which
	// we need to sample from InputTexture as follows:
	//
	// TexelToSample = DimensionsWithinPage - BorderDimensions + (UsefulPageDimensions * PageIndex)
	//
	// where * represents per-component multiplication.
	const uint2 texelToSample = dimensionsWithinPage - CurrentTilingTypeInfo::BORDER_DIMENSIONS + (VirtualTextures::USEFUL_PAGE_DIMENSIONS * pageIndex);
	const float4 sampledInputValue = InputTexture.mips[TilingConstants.InputMipLevel][texelToSample];
	
	// If the input texture is in sRGB format, then we need to manually convert it back after the shader
	// reads it in. This is because UAVs for sRGB textures do not exist.
#ifdef __USING_SRGB_DATA__
	OutputTexture[DTid] = Util::ColorSpace::LinearToSRGB(sampledInputValue);
#else
	OutputTexture[DTid] = sampledInputValue;
#endif
}