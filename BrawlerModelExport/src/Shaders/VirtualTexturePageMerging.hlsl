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

struct ConstantsInfo
{
	uint FirstMipLevelToBeMerged;
	uint NumMipLevelsToMerge;
	uint MaxLogicalSize;
};

Texture2D<float4> InputTexture : register(t0, space0);
ConstantBuffer<ConstantsInfo> MergingConstants : register(b0, space0);

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
	static const uint NO_MIP_LEVEL_SELECTED = uint(-1);
	uint currLaneMipLevelOffset = NO_MIP_LEVEL_SELECTED;
	int2 currLaneOffsetWithinMipLevel = int2(0, 0);
	
	// I'm not sure if there is a way to get which mip level a thread is sampling from in constant time.
	static const uint2 LOGICAL_SIZE_OFFSET = (CurrentTilingTypeInfo::BORDER_DIMENSIONS * 2);
	uint2 currOffset = WaveReadLaneFirst(uint2(0, 0));
	for (uint i = 0; i < MergingConstants.NumMipLevelsToMerge; ++i)
	{
		const uint2 currMipLevelSize = WaveReadLaneFirst(uint2((MergingConstants.MaxLogicalSize >> i) + LOGICAL_SIZE_OFFSET));
		const bool2 withinMipLevelBounds = and(DTid >= currOffset, DTid < (currOffset + currMipLevelSize));
		
		[branch]
		if (currLaneMipLevelOffset == NO_MIP_LEVEL_SELECTED && all(withinMipLevelBounds))
		{
			currLaneMipLevelOffset = i;
			currLaneOffsetWithinMipLevel = (int2(DTid) - currOffset);
		}
		
		// Alternate between adding the offset to the X- and Y-dimensions, starting with the Y-dimension.
		currOffset += WaveReadLaneFirst(uint2(currMipLevelSize.x * (i & 0x1), currMipLevelSize.y * ((i + 1) & 0x1)));
	}
	
	const bool isCurrLaneUseless = (currLaneMipLevelOffset == NO_MIP_LEVEL_SELECTED);
	
	// We need to be careful here, because MergingConstants.MaxLogicalSize >> NO_MIP_LEVEL_SELECTED == 0, and
	// rcp(0) results in a divide-by-zero. In that case, however, we aren't going to be writing to the output
	// texture with that lane, so it doesn't really matter what/where we sample from.
	const float2 inputUV = (float2(currLaneOffsetWithinMipLevel - CurrentTilingTypeInfo::BORDER_DIMENSIONS) + 0.5f) / max(MergingConstants.MaxLogicalSize >> currLaneMipLevelOffset, 1);
	float4 sampledInputValue;
		
	[flatten]
	if(!isCurrLaneUseless)
	{
#ifdef __USING_SRGB_DATA__		
		sampledInputValue = Util::ColorSpace::LinearToSRGB(InputTexture.SampleLevel(PointClampSampler, inputUV, MergingConstants.FirstMipLevelToBeMerged + currLaneMipLevelOffset));
#else
		sampledInputValue = InputTexture.SampleLevel(PointClampSampler, inputUV, MergingConstants.FirstMipLevelToBeMerged + currLaneMipLevelOffset);
#endif
	}
	else
		sampledInputValue = 0.0f;
		
	OutputTexture[DTid] = sampledInputValue;
}