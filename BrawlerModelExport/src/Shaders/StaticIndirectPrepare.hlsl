/*
StaticIndirectPrepare.hlsl is sort of like a merge between UE5 Nanite's triangle cluster system and Doom
Eternal's dynamic draw call merging. The entire Nanite mesh pipeline is *WAY* too complex for a project
like this, but I do like the underlying idea of triangle clusters containing groups of triangles from
a single model instance. I refer the interested reader to "A Deep Dive into Nanite Virtualized Geometry"
from Advances in Real-Time Rendering 2021; it really is an outstanding presentation, and I am sincerely
grateful that Karis et al. were willing to share so many implementation details.

In addition, I really like Doom Eternal's dynamic draw call merging, but their system was designed with
a forward-rendered engine in mind. They devised a system where individual triangles from different meshes
sharing the same PSO get merged into a single indirect index buffer. The relevant data can then be fetched
from StructuredBuffers using the SV_VertexID semantic, since this would have the same value as that which
was specified in the index buffer.

I feel like both of these systems accomplish their goals very well, but they were admittedly designed for
different uses. Unreal Engine 5 is expected to be used with lots of different materials and PSOs, perhaps
by individuals who know next to nothing about real-time rendering. On the contrary, id Software loves to
do the exact opposite by keeping their PSO counts low and combining shading operations into large
ubershaders.

For the Brawler Engine, I wanted the flexibility and possibilities of a deferred engine, but with the knowledge
that the number of PSOs will be small relative to (most) other AAA games. With that in mind, this shader
was created. Its purpose is to create an indirect index buffer whose values contain a triangle cluster ID
and triangle ID within a cluster packed into a single 32-bit integer. This can then be fed to a pixel
shader to write out visibility buffer contents, similar to Nanite's CullRasterize() step.
*/

#include "BindlessDescriptors.hlsli"
#include "GeneralUtil.hlsli"

// A constant buffer is limited to 64 KB of data. In HLSL, arrays in constant buffers are padded such
// that the size of each element in an array is aligned to a 16-byte stride. To make copying to GPU
// memory from C++ code easier, we make each array element a uint4. Due to the HLSL padding rules, this
// also allows us to specify four times as many indices compared to using just uint values with the same
// memory cost.
static const uint MAX_CLUSTER_INDEX_VECTOR_COUNT_PER_BUFFER = 1024;

struct ClusterIndexBuffer
{
    uint4 ClusterIndexArr[MAX_CLUSTER_INDEX_VECTOR_COUNT_PER_BUFFER];
};

struct ViewConstants
{
    float4x4 ViewMatrix;
    float4x4 ViewProjectionMatrix;
	
    float2 ViewDimensions;
    float2 __Pad0;
};

// ================================================================================
// Manual Root Parameters
// ================================================================================

ConstantBuffer<ClusterIndexBuffer> TriangleClusterIndexBufferRootCBV : register(b0, space0);
ConstantBuffer<ViewConstants> PerViewConstants : register(b1, space0);

RWByteAddressBuffer IndirectDrawArgsBuffer : register(u0, space0);
RWByteAddressBuffer VertexBuffer : register(u1, space0);

struct Triangle
{
	float4 VertexAPosition;
	float4 VertexBPosition;
	float4 VertexCPosition;
	
    uint TriangleClusterID;
};

template <typename PointType>
struct GenericBoundingBox
{
	PointType MinPoint;
	PointType MaxPoint;
};

typedef GenericBoundingBox<float2> BoundingBox2D;

Triangle CreateTriangleForThread(in const uint DTid, in const uint groupID, in const uint GTid)
{
	const Buffer<uint> indexBuffer = GetGlobalIndexBuffer();
	
	// TriangleClusterIndexBufferRootCBV is a root CBV, meaning that out-of-bounds accesses to its ClusterIndexArr
	// member will result in undefined behavior. However, since we launch precisely one thread group for each
	// triangle cluster submitted, and since we index into ClusterIndexArr based on the ID of the thread group of
	// a given thread (i.e., SV_GroupID, and not SV_GroupThreadID), we guarantee that we won't get an out-of-bounds
	// access.
	
    const uint triangleClusterBufferArrayIndex = (groupID / 4);
    const uint clusterIDVectorIndex = (DTid & 0x3);
    const uint triangleClusterID = SelectComponent(TriangleClusterIndexBufferRootCBV.ClusterIndexArr[triangleClusterBufferArrayIndex], clusterIDVectorIndex);
	
    Triangle threadTriangle;
    threadTriangle.TriangleClusterID = triangleClusterID;
	
    const uint triangleIDWithinCluster = GTid;
	
	// The offset from within the global index buffer is the global triangle ID * 3. Each cluster has at most 128
	// triangles. If a cluster has less than 128 triangles, then padding is added within the index buffer to match
	// the required value.
	//
	// Because we add this padding, we are able to always get the indices for a triangle within a triangle cluster.
    const uint baseIndexBufferOffset = ((triangleClusterID * MAX_TRIANGLES_PER_TRIANGLE_CLUSTER) + triangleIDWithinCluster) * 3;
	const StructuredBuffer<StaticVertex> globalVertexBuffer = GetGlobalVertexBuffer();
	
	threadTriangle.VertexAPosition = float4(globalVertexBuffer[NonUniformResourceIndex(baseIndexBufferOffset)].PositionAndTangentFrame.xyz, 1.0f);
	threadTriangle.VertexBPosition = float4(globalVertexBuffer[NonUniformResourceIndex(baseIndexBufferOffset + 1)].PositionAndTangentFrame.xyz, 1.0f);
	threadTriangle.VertexCPosition = float4(globalVertexBuffer[NonUniformResourceIndex(baseIndexBufferOffset + 2)].PositionAndTangentFrame.xyz, 1.0f);
	
	return threadTriangle;
}

// ===========================================================================================================
// COMPUTE SHADER CONTRACT
//
// Thread Group Dimensions: 128 x 1 x 1
//
// Description: Launch one thread group for each triangle cluster identified in 
// TriangleClusterIndexBufferRootCBV.
// ===========================================================================================================
[numthreads(128, 1, 1)]
void main(in const uint DTid : SV_DispatchThreadID, in const uint groupID : SV_GroupID, in const uint GTid : SV_GroupThreadID)
{
	Triangle threadTriangle = CreateTriangleForThread(DTid, groupID, GTid);
    const UnpackedTriangleCluster currTriangleCluster = UnpackTriangleCluster(GetTriangleClusterBuffer()[NonUniformResourceIndex(threadTriangle.TriangleClusterID)]);
	
	// Mark this triangle as culled if it doesn't actually belong to the current cluster.
    bool cullTriangle = (GTid >= currTriangleCluster.TriangleCount);
	
	// Perform the 2D homogeneous determinant test to quickly check for orientation and zero-area
	// triangles. To do that, we must first move the vertex positions into homogeneous clip space.
	
    const ModelInstanceData modelInstanceData = GetModelInstanceDataBuffer()[NonUniformResourceIndex(currTriangleCluster.ModelInstanceDataIndex)];
	
    threadTriangle.VertexAPosition *= (modelInstanceData.WorldMatrix * PerViewConstants.ViewProjectionMatrix);
    threadTriangle.VertexBPosition *= (modelInstanceData.WorldMatrix * PerViewConstants.ViewProjectionMatrix);
    threadTriangle.VertexCPosition *= (modelInstanceData.WorldMatrix * PerViewConstants.ViewProjectionMatrix);
	
    cullTriangle = (cullTriangle || determinant(float3x3(threadTriangle.VertexAPosition.xyw, threadTriangle.VertexBPosition.xyw, threadTriangle.VertexCPosition.xyw)) <= 0.0f);
	
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
    {
		// The idea is that we want to edit the VertexCountPerInstance member of the D3D12_DRAW_ARGUMENTS
		// structure represented by IndirectDrawArgsBuffer. We need to use atomic functions because this
		// value is going to be concurrently modified by multiple lanes across waves.
		//
		// To do this, we need to represent the D3D12_DRAW_ARGUMENTS buffer as a RWByteAddressBuffer.
		// Then, we know that the member which we want to modify is 0 bytes away from the start of the structure.
		// It's as ugly as sin, but it seems to be the only way.
		
        IndirectDrawArgsBuffer.InterlockedAdd(0, (numTrianglesToRasterize * 3), baseOutputOffsetForWave);
    }

	// Scalarize the offset at which indices must be written for this wave.
	baseOutputOffsetForWave = WaveReadLaneFirst(baseOutputOffsetForWave);
	
    static const uint NUM_BYTES_OUTPUT_PER_LANE = 24;
	const uint baseOutputOffsetForLane = baseOutputOffsetForWave + (WavePrefixCountBits(rasterizeTriangle) * NUM_BYTES_OUTPUT_PER_LANE);
	
    const uint2 outputVertexA = uint2(threadTriangle.TriangleClusterID, GTid * 3);
    const uint2 outputVertexB = uint2(threadTriangle.TriangleClusterID, (GTid * 3) + 1);
    const uint2 outputVertexC = uint2(threadTriangle.TriangleClusterID, (GTid * 3) + 2);
	
	[branch]
	if (rasterizeTriangle)
    {
        VertexBuffer.Store4(baseOutputOffsetForLane, uint4(outputVertexA, outputVertexB));
        VertexBuffer.Store2(baseOutputOffsetForLane + 16, outputVertexC);
    }
}