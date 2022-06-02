// Yes, this was definitely ripped wholesale from the MiniEngine implementation on GitHub.
// I'm still learning HLSL, you know?

Texture2D<float4> InputTexture : register(t0, space0);
RWTexture2D<float4> OutputMip1 : register(u0, space0);
RWTexture2D<float4> OutputMip2 : register(u1, space0);

SamplerState BilinearClampSampler : register(s0, space0);

struct MipMapGenerationInfo
{
	float2 InverseOutputDimensions;
	uint StartingMipLevel;
	uint OutputMipLevelsCount;
};

ConstantBuffer<MipMapGenerationInfo> MipMapConstants : register(b0, space0);

static const uint THREADS_IN_GROUP = 64;

[numthreads(8, 8, 1)]
void main(in const uint3 DTid : SV_DispatchThreadID, in const uint2 GTid : SV_GroupThreadID)
{
	// Assuming that we are creating X * Y threads, where X and Y are the x- and y-dimensions
	// of OutputMip1, we can do the following:
	
	const float2 inputTextureUV = (DTid.xy + 0.5f) / MipMapConstants.InverseOutputDimensions;

	// Moving from InputTexture -> OutputMip1...
	float4 currOutputValue = InputTexture.SampleLevel(BilinearClampSampler, inputTextureUV, MipMapConstants.StartingMipLevel);
	OutputMip1[DTid.xy] = currOutputValue;
	
	[branch]  // Coherent
	if (MipMapConstants.OutputMipLevelsCount == 1)
        return;
	
	// Rather than reading from OutputMip1, we should keep using currOutputValue. This will
	// allow us to still support hardware that does not have support for typed UAV loads.
	
	// According to the HLSL specifications, we are *NOT* allowed to make any assumptions
	// about SV_GroupIndex or SV_GroupThreadID based on the value provided by a call to
	// WaveGetLaneIndex(). In other words, the ID of a thread within a group is completely
	// unrelated to its index within a wave.* (The source is at 
	// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_Derivatives.html#2d-quads,
	// but I don't know why I have to search the pits of Hell to find such crucial information.
	// Shouldn't this be on the MSDN or something?)
	//
	// *Well, at least for now. Shader Model 6.6 will save us from this torment by actually
	// defining a relationship between SV_GroupThreadID and WaveGetLaneIndex() if special
	// values are used in the numthreads attribute. The source listed above contains more
	// information.
	//
	// However, even with Shader Model 6.0, we *can* share values across nearby threads in
	// a well-defined manner without using LDS by using quad intrinsics! Specifically, a
	// quad is guaranteed to be ordered as follows:
	//
	// -------------> X
	// | [0]	[1]
	// |
	// | [2]	[3]
	// |
	// v
	// Y
	//
	// This works perfectly for downsampling, because we can guarantee that we get the right
	// value for the next mip level.
    float4 adjacentXValue = QuadReadAcrossX(currOutputValue);
    float4 adjacentYValue = QuadReadAcrossY(currOutputValue);
    float4 diagonalValue = QuadReadAcrossDiagonal(currOutputValue);
	
	// Write out the appropriate data, but only once per OutputMip1 quad.
	[branch]
    if (all((GTid.xy & 1) == 0))
        OutputMip2[DTid.xy / 2] = (0.25f * (currOutputValue + adjacentXValue + adjacentYValue + diagonalValue));
	
	// TODO: Add support for further reduction. We should be able to support at least one more
	// downsample within this shader easily.
}