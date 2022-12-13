// NOTE: This shader is part of the environment map pre-filtering process described
// in "Fast Filtering of Reflection Probes" by Manson and Sloan.

Texture2D<float4> InputTextureArr[6] : register(t0, space0);
RWTexture2D<float4> OutputMip1Arr[6] : register(u0, space0);
RWTexture2D<float4> OutputMip2Arr[6] : register(u6, space0);

struct DownsampleConstantsInfo
{
	float StartingMipLevel;
	float InverseStartingMipLevelDimensions;
	float InverseOutputMip1Dimensions;
	float InverseOutputMip2Dimensions;
	uint NumMipLevels;
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

enum class TextureCubeFace
{
	POSITIVE_X = 0,
	NEGATIVE_X = 1,
	POSITIVE_Y = 2,
	NEGATIVE_Y = 3,
	POSITIVE_Z = 4,
	NEGATIVE_Z = 5
};

float3 GetReflectionVector(in const float2 uvCoords, in const TextureCubeFace faceID)
{
	// We refer to the diagram given in the Direct3D 11.3 Specifications which describes the
	// coordinate system for each face of a TextureCube being accessed as a render target.
	
	// TextureCube reflection vectors exist in the [-1, 1]^3 cube, so we need to
	// appropriately scale the uvCoords from [0, 1] to [-1, 1].
	const float2 scaledUVCoords = (2.0f * uvCoords) - 1.0f;
	
	// We can avoid divergence by calculating the reflection vector using homogeneous
	// coordinates [u, v, w]. We store the transformation matrices in a static array
	// which is indexed based on textureCubeFaceIndex, and transform our scaledUVCoords
	// by the transformation matrix. To understand why these matrices were chosen, refer
	// to the Resource Type Illustrations diagram given at the beginning of Section 5 of the
	// Direct3D 11.3 Specifications.
	
	static const float3x3 UV_TRANSFORM_MATRIX_ARR[6] = {
		// TextureCubeFace::POSITIVE_X (+X Face, Index 0)
		float3x3(
			0.0f, 0.0f, -1.0f,
			0.0f, -1.0f, 0.0f,
			1.0f, 0.0f, 0.0f
		),
		
		// TextureCubeFace::NEGATIVE_X (-X Face, Index 1)
		float3x3(
			0.0f, 0.0f, 1.0f,
			0.0f, -1.0f, 0.0f,
			-1.0f, 0.0f, 0.0f
		),
		
		// TextureCubeFace::POSITIVE_Y (+Y Face, Index 2)
		float3x3(
			1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f,
			0.0f, 1.0f, 0.0f
		),
		
		// TextureCubeFace::NEGATIVE_Y (-Y Face, Index 3)
		float3x3(
			1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, -1.0f,
			0.0f, -1.0f, 0.0f
		),
		
		// TextureCubeFace::POSITIVE_Z (+Z Face, Index 4)
		float3x3(
			1.0f, 0.0f, 0.0f,
			0.0f, -1.0f, 0.0f,
			0.0f, 0.0f, 1.0f
		),
		
		// TextureCubeFace::NEGATIVE_Z (-Z Face, Index 5)
		float3x3(
			-1.0f, 0.0f, 0.0f,
			0.0f, -1.0f, 0.0f,
			0.0f, 0.0f, -1.0f
		)
	};

	const uint transformMatrixArrIndex = uint(faceID);
	return mul(float3(scaledUVCoords, 1.0f), UV_TRANSFORM_MATRIX_ARR[transformMatrixArrIndex]);
}

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, 1)]
void main(in const ThreadInfo threadInfo)
{
	const float2 baseTexelCoords = float2(threadInfo.DispatchThreadID * 2);
	const uint textureArrayIndex = threadInfo.GroupID.z;
	
	// Each lane is responsible for a single 2x2 quad in the input texture. Refer to the comments
	// above to understand how these offset values are derived.
	const float texelCoordsXOffset = (threadInfo.DispatchThreadID.x % 2 == 0 ? 1.25f : 0.75f);
	const float texelCoordsYOffset = (threadInfo.DispatchThreadID.y % 2 == 0 ? 1.25f : 0.75f);
	
	const float2 uvCoords = ((baseTexelCoords + float2(texelCoordsXOffset, texelCoordsYOffset)) * DownsampleConstants.InverseStartingMipLevelDimensions);
	
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
	const float2 outputMip1UVCoords = ((float2(threadInfo.DispatchThreadID) + 0.5f) * DownsampleConstants.InverseOutputMip1Dimensions);
	const float3 currLaneMip1ReflectionVector = GetReflectionVector(outputMip1UVCoords, TextureCubeFace(textureArrayIndex));
	
	float jacobianValue = (1.0f / pow(dot(currLaneMip1ReflectionVector, currLaneMip1ReflectionVector), 1.5f));
	float w = (0.125f * (1.0f + jacobianValue));
	
	float4 currLaneSampledValue = InputTextureArr[NonUniformResourceIndex(textureArrayIndex)].SampleLevel(BilinearClampSampler, uvCoords, DownsampleConstants.StartingMipLevel) * w;
	
	OutputMip1Arr[NonUniformResourceIndex(textureArrayIndex)][threadInfo.DispatchThreadID] = currLaneSampledValue;
	
	// TODO: Implement writes for OutputMip2!
}