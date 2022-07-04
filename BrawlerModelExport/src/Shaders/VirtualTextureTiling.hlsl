#include "VirtualTextureConstants.hlsli"
#include "ColorSpaceUtil.hlsli"

#if defined(__TILING_MODE_POINT_FILTER__)
typedef VirtualTextures::TilingTypeInfo<VirtualTextures::POINT_FILTER_TILING_MODE> CurrentTilingTypeInfo;
#elif defined(__TILING_MODE_BILINEAR_FILTER__)
typedef VirtualTextures::TilingTypeInfo<VirtualTextures::BILINEAR_FILTER_TILING_MODE> CurrentTilingTypeInfo;
#elif defined(__TILING_MODE_TRILINEAR_FILTER__)
typedef VirtualTextures::TilingTypeInfo<VirtualTextures::TRILINEAR_FILTER_TILING_MODE> CurrentTilingTypeInfo;
#elif defined(__TILING_MODE_ANISOTROPIC_8X_FILTER__)
typedef VirtualTextures::TilingTypeInfo<VirtualTextures::ANISOTROPIC_8X_FILTER_TILING_MODE> CurrentTilingTypeInfo;
#else
#error "ERROR: A tiling mode *MUST* be specified as a preprocessor definition! (Valid modes are __TILING_MODE_POINT_FILTER__, __TILING_MODE_BILINEAR_FILTER__, __TILING_MODE_TRILINEAR_FILTER__, and __TILING_MODE_ANISOTROPIC_8X_FILTER__.)"
#endif

struct TilingInfo
{
	uint2 StartingLogicalCoordinates;
	uint OutputPageCount;
};
	
struct MipLevelInfo
{
	uint InputMipLevel;
	uint MipLevelLogicalSize;
	uint2 __Pad0;
};

Texture2D<float4> InputTexture : register(t0, space0);

ConstantBuffer<TilingInfo> TilingConstants : register(b0, space0);
ConstantBuffer<MipLevelInfo> MipLevelConstants : register(b1, space0);

RWTexture2D<float4> OutputPage0 : register(u0, space0);
RWTexture2D<float4> OutputPage1 : register(u1, space0);
RWTexture2D<float4> OutputPage2 : register(u2, space0);
RWTexture2D<float4> OutputPage3 : register(u3, space0);

SamplerState PointClampSampler : register(s0, space0);
	
namespace IMPL
{
	// There's no if-constexpr in HLSL, so this is what we have to do. *sigh...*
	template <uint OutputPageIndex>
	struct StartCoordsAdjuster
	{
		inline static uint2 GetAdjustedStartCoordinates()
		{
			uint2 startCoordinates = TilingConstants.StartingLogicalCoordinates;
			const uint adjustedXCoord = mad(VirtualTextures::USEFUL_PAGE_DIMENSIONS.x, OutputPageIndex, startCoordinates.x);
			
			startCoordinates.x = (adjustedXCoord & (MipLevelConstants.MipLevelLogicalSize - 1));
			startCoordinates.y = mad(VirtualTextures::USEFUL_PAGE_DIMENSIONS.y, (adjustedXCoord >> firstbitlow(MipLevelConstants.MipLevelLogicalSize)), startCoordinates.y);
				
			return startCoordinates;
		}
	};
	
	template <>
	struct StartCoordsAdjuster<0>
	{
		inline static uint2  GetAdjustedStartCoordinates()
		{
			return TilingConstants.StartingLogicalCoordinates;
		}
	};
}
	
namespace IMPL
{
	template <uint OutputPageIndex>
	struct OutputPageWriter
	{};
		
	template <>
	struct OutputPageWriter<0>
	{
		inline static void WriteToOutputPage(in const uint2 outputCoordinates, in const float4 valueToWrite)
		{
			OutputPage0[outputCoordinates] = valueToWrite;
		}
	};
		
	template <>
	struct OutputPageWriter<1>
	{
		inline static void WriteToOutputPage(in const uint2 outputCoordinates, in const float4 valueToWrite)
		{
			OutputPage1[outputCoordinates] = valueToWrite;
		}
	};
	
	template <>
	struct OutputPageWriter<2>
	{
		inline static void WriteToOutputPage(in const uint2 outputCoordinates, in const float4 valueToWrite)
		{
			OutputPage2[outputCoordinates] = valueToWrite;
		}
	};
	
	template <>
	struct OutputPageWriter<3>
	{
		inline static void WriteToOutputPage(in const uint2 outputCoordinates, in const float4 valueToWrite)
		{
			OutputPage3[outputCoordinates] = valueToWrite;
		}
	};
}
	
template <uint OutputPageIndex>
void BeginOutputPageWrite(in const uint2 DTid)
{
	const uint2 adjustedStartCoordinates = WaveReadLaneFirst(IMPL::StartCoordsAdjuster<OutputPageIndex>::GetAdjustedStartCoordinates());
	const uint2 texelSampleCoordinates = adjustedStartCoordinates + DTid - CurrentTilingTypeInfo::BORDER_DIMENSIONS;
		
	// AMD suggests to avoid using rcp(), since it runs at quarter-rate as a transcendental instruction. Is
	// (1.0f / x) really faster than rcp(x), then?
	const float2 inputUV = (float2(texelSampleCoordinates) + 0.5f) * (1.0f / MipLevelConstants.MipLevelLogicalSize);
	
#ifdef __USING_SRGB_DATA__
	const float4 sampledInputValue = Util::ColorSpace::LinearToSRGB(InputTexture.SampleLevel(PointClampSampler, inputUV, MipLevelConstants.InputMipLevel));
#else
	const float4 sampledInputValue = InputTexture.SampleLevel(PointClampSampler, inputUV, MipLevelConstants.InputMipLevel);
#endif
		
	IMPL::OutputPageWriter<OutputPageIndex>::WriteToOutputPage(texelSampleCoordinates + CurrentTilingTypeInfo::BORDER_DIMENSIONS, sampledInputValue);
}

// ===========================================================================================================
// COMPUTE SHADER CONTRACT
//
// Thread Group Dimensions: 8 x 8 x 1
//
// Description: Launch one thread for each texel in a page.
// ===========================================================================================================
[numthreads(8, 8, 1)]
void main(in const uint2 DTid : SV_DispatchThreadID)
{
	BeginOutputPageWrite<0>(DTid);
		
	// Coherent
	[branch]
	if (TilingConstants.OutputPageCount == 1)
		return;
		
	BeginOutputPageWrite<1>(DTid);
		
	// Coherent
	[branch]
	if (TilingConstants.OutputPageCount == 2)
		return;
		
	BeginOutputPageWrite<2>(DTid);
		
	// Coherent
	[branch]
	if (TilingConstants.OutputPageCount == 3)
		return;
		
	BeginOutputPageWrite<3>(DTid);
}