// We might also be able to use this exact same struct for skinned data, too! The difference there
// is that we would be creating a separate buffer containing the bone indices and weights.
struct StaticVertex
{
	float4 PositionAndTangentFrame;
	
	float2 UVCoords;
	uint2 __Pad0;
};

// NOTE: *ALL* matrices are made implicitly row-major by the Brawler Shader Compiler.

struct BindlessIndexer
{
	uint Index;
};

struct ModelInstanceData
{
    float4x4 WorldMatrix;
};

struct ViewConstants
{
	float4x4 ViewMatrix;
	float4x4 ViewProjectionMatrix;
	
	float2 ViewDimensions;
	float2 __Pad0;
};

// NOTE: For Doom Eternal's dynamic draw call merging, the idea is *NOT* to make an actual index
// buffer, as the slides seem to suggest. Rather, one should dynamically create a vertex buffer for 
// a non-indexed draw. The vertices in this buffer contain only one attribute: an index into the
// global vertex StructuredBuffer containing the actual data for said vertex.
//
// id Software did something different, but unless there are some crazy hardware shenanigans which
// make my method less performant, it should work just as well.

// ================================================================================
// Bindless Types v
//
// NOTE: By tradition, spaces greater than space0 are reserved for bindless SRV
// declarations.
// ================================================================================

StructuredBuffer<StaticVertex> Bindless_GlobalVertexBuffer[] : register(t0, space1);
Buffer<uint> Bindless_GlobalIndexBuffer[] : register(t0, space2);
StructuredBuffer<ModelInstanceData> Bindless_GlobalModelInstanceDataBuffer[] : register(t0, space3);

// ================================================================================
// ^ Bindless Types | Manual Root Parameters v
// ================================================================================

ConstantBuffer<BindlessIndexer> BindlessGlobalIndexBufferOffset : register(b0, space0);
ConstantBuffer<ViewConstants> PerViewConstants : register(b1, space0);

RWBuffer<uint> IndexCount : register(u0, space0);
RWBuffer<uint> IndexBuffer : register(u1, space0);

struct Triangle
{
	float4 VertexAPosition;
	uint VertexAGlobalVBIndex;
	
	float4 VertexBPosition;
	uint VertexBGlobalVBIndex;
	
	float4 VertexCPosition;
	uint VertexCGlobalVBIndex;
};

template <typename PointType>
struct GenericBoundingBox
{
	PointType MinPoint;
	PointType MaxPoint;
};

typedef GenericBoundingBox<float2> BoundingBox2D;

StructuredBuffer<StaticVertex> GetGlobalVertexBuffer()
{
	static const uint GLOBAL_VERTEX_BUFFER_BINDLESS_INDEX = 0;
	
	return Bindless_GlobalVertexBuffer[GLOBAL_VERTEX_BUFFER_BINDLESS_INDEX];
}

Buffer<uint> GetGlobalIndexBuffer()
{
	static const uint GLOBAL_INDEX_BUFFER_BINDLESS_INDEX = 1;
	
	return Bindless_GlobalIndexBuffer[GLOBAL_INDEX_BUFFER_BINDLESS_INDEX];
}

Triangle CreateTriangleForThread(in const uint DTid)
{
	const Buffer<uint> indexBuffer = GetGlobalIndexBuffer();
	
	// indexBuffer is in the bindless SRV table, so its descriptor table is marked
	// DESCRIPTORS_VOLATILE. This means that out-of-bounds accesses are guaranteed to produce
	// defined results, so we don't need to do a check for that here.
	
	const uint baseIndexBufferOffset = BindlessGlobalIndexBufferOffset.Index + (DTid * 3);
	const StructuredBuffer<StaticVertex> globalVertexBuffer = GetGlobalVertexBuffer();
	
	Triangle threadTriangle;
	threadTriangle.VertexAGlobalVBIndex = baseIndexBufferOffset;
	threadTriangle.VertexAPosition = float4(globalVertexBuffer[NonUniformResourceIndex(threadTriangle.VertexAGlobalVBIndex)].PositionAndTangentFrame.xyz, 1.0f);
	
	threadTriangle.VertexBGlobalVBIndex = baseIndexBufferOffset + 1;
	threadTriangle.VertexBPosition = float4(globalVertexBuffer[NonUniformResourceIndex(threadTriangle.VertexBGlobalVBIndex)].PositionAndTangentFrame.xyz, 1.0f);
	
	threadTriangle.VertexCGlobalVBIndex = baseIndexBufferOffset + 2;
	threadTriangle.VertexCPosition = float4(globalVertexBuffer[NonUniformResourceIndex(threadTriangle.VertexCGlobalVBIndex)].PositionAndTangentFrame.xyz, 1.0f);
	
	return threadTriangle;
}

[numthreads(64, 1, 1)]
void main(in const uint3 DTid : SV_DispatchThreadID)
{
	// The original implementation in "Optimizing the Graphics Pipeline with Compute" uses the
	// GPU to do the normal bounding cone culling. In my opinion, this doesn't make much sense.
	// If we can calculate the result quickly enough on the CPU once for an entire group of 256
	// triangles, then why would we want to do it on the GPU 256 times, once for each triangle?
	
	Triangle threadTriangle = CreateTriangleForThread(DTid.x);
	
	// Perform the 2D homogeneous determinant test to quickly check for orientation and zero-area
	// triangles. To do that, we must first move the vertex positions into homogeneous clip space.
	
	threadTriangle.VertexAPosition *= PerViewConstants.WorldViewProjectionMatrix;
	threadTriangle.VertexBPosition *= PerViewConstants.WorldViewProjectionMatrix;
	threadTriangle.VertexCPosition *= PerViewConstants.WorldViewProjectionMatrix;
	
	bool cullTriangle = determinant(float3x3(threadTriangle.VertexAPosition.xyw, threadTriangle.VertexBPosition.xyw, threadTriangle.VertexCPosition.xyw)) <= 0.0f;
	
	// [Coherent]
	[branch]
	if (WaveActiveAllTrue(cullTriangle))
		return;
	
	// =====================================================================================================
	//
	// TODO: In "Optimizing the Graphics Pipeline with Compute," Wihlidal performs occlusion culling
	// as the next step. This is an effective culling mechanism, but it requires the availability
	// of a depth buffer.
	//
	// The Brawler Engine uses (or, at the time of writing this, plans to use) a bindless deferred
	// rendering engine model, similar to MJP's BindlessDeferred D3D12 sample. This means that the
	// pixel shader is going to be very simple in comparison to a forward-rendered engine or even a
	// traditional deferred engine.
	//
	// The question that remains is as follows: Should we still generate a depth buffer and perform
	// occlusion culling, knowing that the cost of wasted vertex lanes will not be significant relative
	// to other engines?
	//
	// Only profiling will tell. For now, we will skip occlusion culling and see how badly performance
	// is affected.
	//
	// Keep in mind, however, that we don't need to necessarily perform a full Z-prepass. We could
	// do a partial Z-prepass containing, say, only static objects. Another idea would be to re-use
	// a depth buffer from the previous frame for occlusion culling. This would need to be conservative,
	// however, and triangles occluded by dynamic objects from the previous frame cannot necessarily
	// be considered occluded. We could use the stencil buffer to denote static vs. dynamic objects in
	// the depth buffer for that purpose, but I'm not sure how that would affect depth buffer compression.
	//
	// =====================================================================================================
	
	// Complete the perspective divide and bring the positions into NDC space.
	threadTriangle.VertexAPosition.xyz /= threadTriangle.VertexAPosition.w;
	threadTriangle.VertexBPosition.xyz /= threadTriangle.VertexBPosition.w;
	threadTriangle.VertexCPosition.xyz /= threadTriangle.VertexCPosition.w;
	
	// Create the screen-space bounding box.
	const float2 vertexAPositionSS = threadTriangle.VertexAPosition.xy;
	const float2 vertexBPositionSS = threadTriangle.VertexBPosition.xy;
	const float2 vertexCPositionSS = threadTriangle.VertexCPosition.xy;
	
	BoundingBox2D triangleBoundingBoxSS;
	triangleBoundingBoxSS.MinPoint = min(vertexAPositionSS, min(vertexBPositionSS, vertexCPositionSS));
	triangleBoundingBoxSS.MaxPoint = max(vertexAPositionSS, max(vertexBPositionSS, vertexCPositionSS));
	
	// Converting from NDC to screen space...
	triangleBoundingBoxSS.MinPoint = mad(triangleBoundingBoxSS.MinPoint, 0.5f, 0.5f);
	triangleBoundingBoxSS.MinPoint.y = 1.0f - triangleBoundingBoxSS.MinPoint.y;
	
	triangleBoundingBoxSS.MaxPoint = mad(triangleBoundingBoxSS.MaxPoint, 0.5f, 0.5f);
	triangleBoundingBoxSS.MaxPoint.y = 1.0f - triangleBoundingBoxSS.MaxPoint.y;
	
	// Perform frustum culling in screen space. Unlike in "Optimizing the Graphics Pipeline with Compute,"
	// we do this *before* the small primitive test since that test requires the points to be converted into
	// pixel units, but this one can be done in fractional screen space units.
	cullTriangle = (cullTriangle || any(bool4(triangleBoundingBoxSS.MaxPoint.x < 0.0f, triangleBoundingBoxSS.MinPoint.x > 1.0f, triangleBoundingBoxSS.MaxPoint.y < 0.0f, triangleBoundingBoxSS.MinPoint.y > 1.0f)));
	
	// Convert the pixels to actual units and perform the approximate small primitive test.
	triangleBoundingBoxSS.MinPoint *= PerViewConstants.ViewDimensions;
	triangleBoundingBoxSS.MaxPoint *= PerViewConstants.ViewDimensions;
	
	cullTriangle = (cullTriangle || any(round(triangleBoundingBoxSS.MinPoint) == round(triangleBoundingBoxSS.MaxPoint)));
	
	// [Coherent]
	[branch]
	if (WaveActiveAllTrue(cullTriangle))
		return;
	
	// Scalarize the number of triangles in the current wave which are *NOT* going to be culled.
	const bool rasterizeTriangle = !cullTriangle;
	const uint numTrianglesToRasterize = WaveActiveCountBits(rasterizeTriangle);
	
	// Reserve (numTrianglesToRasterize * 3) indices from IndexBuffer.
	uint baseOutputOffsetForWave;
	
	[branch]
	if (WaveIsFirstLane())
		InterlockedAdd(IndexCount[0], (numTrianglesToRasterize * 3), baseOutputOffsetForWave);

	// Scalarize the offset at which indices must be written.
	baseOutputOffsetForWave = WaveReadLaneFirst(baseOutputOffsetForWave);
	
    const uint baseOutputOffsetForLane = baseOutputOffsetForWave + WavePrefixCountBits(rasterizeTriangle);
	
	[branch]
	if (rasterizeTriangle)
    {
        IndexBuffer[NonUniformResourceIndex(baseOutputOffsetForLane)] = threadTriangle.VertexAGlobalVBIndex;
        IndexBuffer[NonUniformResourceIndex(baseOutputOffsetForLane + 1)] = threadTriangle.VertexBGlobalVBIndex;
        IndexBuffer[NonUniformResourceIndex(baseOutputOffsetForLane + 2)] = threadTriangle.VertexCGlobalVBIndex;
    }
}