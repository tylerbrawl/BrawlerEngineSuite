#include "TextureCubeFace.hlsli"

// NOTE: This shader is part of the environment map pre-filtering process described
// in "Fast Filtering of Reflection Probes" by Manson and Sloan.

Texture2D<float4> InputTextureArr[6] : register(t0, space0);
RWTexture2D<float4> OutputTextureArr[6] : register(u0, space0);

struct DownsampleConstantsInfo
{
	float StartingMipLevel;
	float InverseStartingMipLevelDimensions;
};

ConstantBuffer<DownsampleConstantsInfo> DownsampleConstants : register(b0, space0);

SamplerState BilinearClampSampler : register(s0, space0);

struct ThreadInfo
{
	uint2 DispatchThreadID : SV_DispatchThreadID;
	uint3 GroupID : SV_GroupID;
};

static const uint NUM_THREADS_X = 8;
static const uint NUM_THREADS_Y = 8;

static const uint NUM_THREADS_IN_THREAD_GROUP = (NUM_THREADS_X * NUM_THREADS_Y);

// Environment map pre-filtering is implemented as a two-step process. The first step
// consists of a downsampling pass, and this is what is performed by this shader.
//
// In "Fast Filtering of Reflection Probes", Manson and Sloan use a quadratic b-spline
// filter to perform the downsampling. The 1D filter kernel for this is [1/8 3/8 3/8 1/8],
// and the 2D kernel is the 4x4 tensor product of this kernel (that is, the transpose of
// the kernel multiplied by itself via matrix multiplication):
//
// mul(transpose([1/8 3/8 3/8 1/8]), [1/8 3/8 3/8 1/8]) =
//
// [ 1/64   3/64   3/64   1/64]
// | 3/64   9/64   9/64   3/64|
// | 3/64   9/64   9/64   3/64|
// [ 1/64   3/64   3/64   1/64]
//
// Naively, this could be implemented using 16 nearest-neighbor samples, but that would
// be horrendous for performance. Instead, if we are (really) clever with our texture
// sampling coordinates, we can get away with doing only four bilinear samples to achieve
// the same result.
//
// The key is to "trick" the hardware into using weight values for each texel corresponding
// to their overall contribution relative to the lowest weighted texel in the quad. When we
// downsample, we want to move the result for each 2x2 quad into the associated texel in
// the next mip level. Suppose this mip level is laid out like so, where each labeled texel
// has four corresponding texels in the previous mip level:
//
// ---------
// | A | B |
// |-------|
// | C | D |
// ---------
//
// What follows is a breakdown of the calculation of the texture coordinates for each output
// texel. (You know, it would've been nice if Mason and Sloan provided these in the paper...)
//
// EDIT: I'm blind, apparently. You can get the source code for these operations at
// https://research.activision.com/publications/archives/fast-filtering-of-reflection-probes.
	
// =============================================================================================================================
	
// Texel A
//
// Associated Kernel:
// [ 1/64   3/64 ]
// [ 3/64   9/64 ]
//
// We want the GPU to perform the following calculation:
// (1/64) * Input[x, y] + (3/64) * Input[x + 1, y] + (3/64) * Input[x, y + 1] + (9/64) * Input[x + 1, y + 1]
//
// = (1/4) * ((1/16) * Input[x, y] + (3/16) * Input[x + 1, y] + (3/16) * Input[x, y + 1] + (9/16) * Input[x + 1, y + 1])
//
// = (1/4) * ((1/4) * a + (3/4) * b)
//
// where:
//   a = (1/4) * Input[x, y] + (3/4) * Input[x + 1, y]
//   b = (1/4) * Input[x, y + 1] + (3/4) * Input[x + 1, y + 1]
//
// So, we need to choose our uv coordinates to use bilinear filter weights of t = 0.25 in the x-direction
// and t = 0.25 in the y-direction, and then multiply the sampled value by (1/4). This can be accomplished
// with texture coordinate offsets of (x + 1.25f) and (y + 1.25f). 
//
// (If you are unsure as to how the t values are calculated, refer to the Direct3D 11.3 Specifications, Section 
// 7.18.8: Linear Sample Addressing.)
	
// =============================================================================================================================
	
// Texel B
//
// Associated Kernel:
// [ 3/64   1/64 ]
// [ 9/64   3/64 ]
//
// We want the GPU to perform the following calculation:
// (3/64) * Input[x, y] + (1/64) * Input[x + 1, y] + (9/64) * Input[x, y + 1] + (3/64) * Input[x + 1, y + 1]
//
// = (1/4) * ((3/16) * Input[x, y] + (1/16) * Input[x + 1, y] + (9/16) * Input[x, y + 1] + (3/16) * Input[x + 1, y + 1])
//
// = (1/4) * ((1/4) * a + (3/4) * b)
//
// where:
//   a = (3/4) * Input[x, y] + (1/4) * Input[x + 1, y]
//   b = (3/4) * Input[x, y + 1] + (1/4) * Input[x + 1, y + 1]
//
// So, we need to choose our uv coordinates to use bilinear filter weights of t = 0.75 in the x-direction
// and t = 0.25 in the y-direction, and then multiply the sampled value by (1/4). This can be accomplished
// with texture coordinate offsets of (x + 0.75f) and (y + 1.25f).
	
// =============================================================================================================================
	
// Texel C
//
// Associated Kernel:
// [ 3/64   9/64 ]
// [ 1/64   3/64 ]
//
// We want the GPU to perform the following calculation:
// (3/64) * Input[x, y] + (9/64) * Input[x + 1, y] + (1/64) * Input[x, y + 1] + (3/64) * Input[x + 1, y + 1]
//
// = (1/4) * ((3/16) * Input[x, y] + (9/16) * Input[x + 1, y] + (1/16) * Input[x, y + 1] + (3/16) * Input[x + 1, y + 1])
//
// = (1/4) * ((3/4) * a + (1/4) * b)
//
// where:
//   a = (1/4) * Input[x, y] + (3/4) * Input[x + 1, y]
//   b = (1/4) * Input[x, y + 1] + (3/4) * Input[x + 1, y + 1]
//
// So, we need to choose our uv coordinates to use bilinear filter weights of t = 0.25 in the x-direction
// and t = 0.75 in the y-direction, and then multiply the sampled value by (1/4). This can be accomplished
// with texture coordinate offsets of (x + 1.25f) and (y + 0.75f).
	
// =============================================================================================================================
	
// Texel D
//
// Associated Kernel:
// [ 9/64   3/64 ]
// [ 3/64   1/64 ]
//
// We want the GPU to perform the following calculation:
// (9/64) * Input[x, y] + (3/64) * Input[x + 1, y] + (3/64) * Input[x, y + 1] + (1/64) * Input[x + 1, y + 1]
//
// = (1/4) * ((9/16) * Input[x, y] + (3/16) * Input[x + 1, y] + (3/16) * Input[x, y + 1] + (1/16) * Input[x + 1, y + 1])
//
// = (1/4) * ((3/4) * a + (1/4) * b)
//
// where:
//   a = (3/4) * Input[x, y] + (1/4) * Input[x + 1, y]
//   b = (3/4) * Input[x, y + 1] + (1/4) * Input[x + 1, y + 1]
//
// So, we need to choose our uv coordinates to use bilinear filter weights of t = 0.75 in the x-direction
// and t = 0.75 in the y-direction, and then multiply the sampled value by (1/4). This can be accomplished
// with texture coordinate offsets of (x + 0.75f) and (y + 0.75f).

struct EnvironmentMapSampleInfo
{
	float2 TexelAUVCoords;
	float2 TexelBUVCoords;
	float2 TexelCUVCoords;
	float2 TexelDUVCoords;
	
	BrawlerHLSL::TextureCubeFace FaceID;
	
	float4 Weights;
};

float CalculateJacobian(in const float2 uvCoords)
{
	// The Jacobian J(x, y, z) maps from a [-1, 1]^3 cube to a unit sphere, and is
	// defined as follows:
	//
	// J(x, y, z) = 1 / pow(x^2 + y^2 + z^2, 3/2)
	//
	// Since we are always sampling a location on one of the faces of the cube, one
	// of x, y, or z will always be +/- 1, and thus its squared value will be 1.
	// The remaining coordinates come directly from the uvCoords after being scaled
	// to the [-1, 1] cube space.
	
	const float2 scaledUVCoords = (2.0f * uvCoords) - 1.0f;
	const float cubeCoordsDotProduct = dot(scaledUVCoords, scaledUVCoords) + 1.0f;
	
	return (1.0f / pow(cubeCoordsDotProduct, 1.5f));
}

void CalculateSampleWeights(inout EnvironmentMapSampleInfo sampleInfo)
{
	// In the paper, Mason and Sloan mention that quadratic b-splines are smooth, but the projection
	// of a constant function over a sphere onto a cubemap is not smooth between faces. To counteract
	// this, they weight the samples using the weight w calculated as follows:
	//
	// w = (1/2) * (1 + J(x, y, z))
	// J(x, y, z) = 1 / pow(x^2 + y^2 + z^2, 3/2)
	//
	// where J(x, y, z) is the Jacobian projection function mapping from a [-1, 1]^3 cube to a unit
	// sphere.
	//
	// The strictly correct way to do the weighting would be to weight each individual value in the
	// 4x4 filter kernel, but doing this prevents us from using the hardware's bilinear filtering
	// to accelerate the sampling process. Instead, Mason and Sloan scale the four bilinear samples
	// by w. This results in subtle non-smoothness over the function after projection onto the
	// sphere, but the errors introduced were deemed a small cost to pay for the ability to use
	// bilinear filtering.
	//
	// We will do the same thing as they did. However, for our w value, we use (1/4) * (1/2) = (1/8)
	// instead of (1/2) because we need to multiply our sampled values by 1/4, as discussed in the
	// comments regarding texel coordinate calculation.
	
	const float4 partialWeights = float4(
		CalculateJacobian(sampleInfo.TexelAUVCoords),
		CalculateJacobian(sampleInfo.TexelBUVCoords),
		CalculateJacobian(sampleInfo.TexelCUVCoords),
		CalculateJacobian(sampleInfo.TexelDUVCoords)
	) + 1.0f;
	
	// The sum of all of the weights, S, is as follows:
	//
	// S = (1/2) * (1 + J_1) + (1/2) * (1 + J_2) + (1/2) * (1 + J_3) + (1/2) * (1 + J_4)
	//
	//   = (1/2) * ((1 + J_1) + (1 + J_2) + (1 + J_3) + (1 + J_4))
	//
	// where J_X represents the Jacobian for texel X. You'll notice a missing (1/2) in
	// the calculation of partialWeightsSum. This is not an oversight: We factor out a
	// (1/2) in both each partial weight and the sum of the partial weights when
	// calculating the actual weight values.
	
	// The unoptimized version of the weights calculation might look something like
	// this:
	//
	// 1. Convert each value in partialWeights to w = (1/2) * (1 + J(x, y, z)):
	// partialWeights *= 0.5f;  
	// 
	// 2. Calculate partialWeightsSum by adding all components of partialWeights:
	// partialWeightsSum = (partialWeights.x + partialWeights.y + partialWeights.z + partialWeights.w);
	//
	// 3. Scale each partialWeight by the sum of all weights:
	// sampleInfo.Weights = (partialWeights / partialWeightsSum)
	//
	// 4. Multiply each weight by (1/4) to account for the factor described in the comments
	// above:
	// sampleInfo.Weights *= 0.25f;
	//
	// To make things more efficient, however, we simply skip Step 1 (0.5x / 0.5y = x / y),
	// combine the contributions of Steps 2 and 4 into a single scalar value, and take the 
	// inverse of the sum to use multiplication instead of division when possible. This reduces
	// instruction count while remaining mathematically correct, but it makes the calculations
	// more difficult to understand (hence these comments).
	
	const float inversePartialWeightsSum = 0.25f / (partialWeights.x + partialWeights.y + partialWeights.z + partialWeights.w);
	sampleInfo.Weights = (partialWeights * inversePartialWeightsSum);
}

float4 SampleEnvironmentMap(in const EnvironmentMapSampleInfo sampleInfo)
{
	Texture2D<float4> inputTexture = InputTextureArr[NonUniformResourceIndex(uint(sampleInfo.FaceID))];
	
	float4 currLaneSampledValue = (inputTexture.SampleLevel(BilinearClampSampler, sampleInfo.TexelAUVCoords, DownsampleConstants.StartingMipLevel) * sampleInfo.Weights.x);
	currLaneSampledValue += (inputTexture.SampleLevel(BilinearClampSampler, sampleInfo.TexelBUVCoords, DownsampleConstants.StartingMipLevel) * sampleInfo.Weights.y);
	currLaneSampledValue += (inputTexture.SampleLevel(BilinearClampSampler, sampleInfo.TexelCUVCoords, DownsampleConstants.StartingMipLevel) * sampleInfo.Weights.z);
	currLaneSampledValue += (inputTexture.SampleLevel(BilinearClampSampler, sampleInfo.TexelDUVCoords, DownsampleConstants.StartingMipLevel) * sampleInfo.Weights.w);
	
	return currLaneSampledValue;
}

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, 1)]
void main(in const ThreadInfo threadInfo)
{
	const float2 baseTexelCoords = float2(threadInfo.DispatchThreadID * 2);
	
	EnvironmentMapSampleInfo sampleInfo;
	sampleInfo.TexelAUVCoords = ((baseTexelCoords + float2(1.25f, 1.25f)) * DownsampleConstants.InverseStartingMipLevelDimensions);
	sampleInfo.TexelBUVCoords = ((baseTexelCoords + float2(0.75f, 1.25f)) * DownsampleConstants.InverseStartingMipLevelDimensions);
	sampleInfo.TexelCUVCoords = ((baseTexelCoords + float2(1.25f, 0.75f)) * DownsampleConstants.InverseStartingMipLevelDimensions);
	sampleInfo.TexelDUVCoords = ((baseTexelCoords + float2(0.75f, 0.75f)) * DownsampleConstants.InverseStartingMipLevelDimensions);
	sampleInfo.FaceID = BrawlerHLSL::TextureCubeFace(threadInfo.GroupID.z);
	
	CalculateSampleWeights(sampleInfo);
	
	RWTexture2D<float4> outputTexture = OutputTextureArr[NonUniformResourceIndex(threadInfo.GroupID.z)];
	outputTexture[threadInfo.DispatchThreadID] = SampleEnvironmentMap(sampleInfo);
}