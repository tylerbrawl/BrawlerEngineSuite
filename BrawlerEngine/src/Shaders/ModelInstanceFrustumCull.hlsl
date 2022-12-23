#include "BindlessDescriptors.hlsli"
#include "MeshTypes.hlsli"
#include "TransformUtil.hlsli"
#include "ViewTypes.hlsli"

// Our current GPU draw submission system uses a non-indexed indirect draw with dynamically generated 
// vertex buffers. These "vertex buffers," represented by OutputGlobalVBIndexBuffer and
// OutputDescriptorBufferIndicesBuffer, contain indices into bindless SRV buffers which are accessed
// during shading to get the actual vertex data, along with material data.
//
// This isn't a particularly smart way to do this. id Software describes a better system which they
// used for Doom: Eternal in the SIGGRAPH 2020 presentation "Rendering the Hellscape of Doom: Eternal."
// Future work would be to alter this system to be more in line with what they do.

struct ModelInstanceFrustumCullConstantsInfo
{
	uint ViewID;
};

ConstantBuffer<ModelInstanceFrustumCullConstantsInfo> ModelInstanceFrustumCullConstants : register(b0, space0);

RWBuffer<uint> OutputGlobalVBIndexBuffer : register(u0, space0);
RWBuffer<uint> OutputDescriptorBufferIndicesBuffer : register(u1, space0);

// The RWByteAddressBuffer OutputIndirectArgumentsBuffer has the following layout:
//
// struct D3D12_DRAW_ARGUMENTS
// {
//     std::uint32_t VertexCountPerInstance;
//     std::uint32_t InstanceCount;
//     std::uint32_t StartVertexLocation;
//     std::uint32_t StartInstanceLocation;
// };
//
// Assuming that the UAV is cleared on the GPU timeline prior to execution of the shader, we are
// primarily interested in updating the VertexCountPerInstance field of the structure. (We do,
// however, also need to make sure that InstanceCount is set to one.) We need atomic operations for 
// this, which is why we use a RWByteAddressBuffer, rather than a RWStructuredBuffer.
RWByteAddressBuffer OutputIndirectArgumentsBuffer : register(u2, space0);

struct ThreadInfo
{
	uint DispatchThreadID : SV_DispatchThreadID;
	uint GroupID : SV_GroupID;
	uint GroupIndex : SV_GroupIndex;
};

static const uint NUM_THREADS_X = 64;

static const uint NUM_THREADS_IN_THREAD_GROUP = NUM_THREADS_X;

bool IsPointWithinViewFrustum(in const float4 positionCS)
{
	// A position v = [x y z w] in homogeneous clip space is in the view frustum iff the following 
	// constraints hold:
	//
	//   - -w <= x <= w
	//   - -w <= y <= w
	//   -  0 <= z <= w
	//
	// This reflects the fact that after the perspective divide, a position v' = [x y z] in NDC
	// space is visible iff the following constraints hold:
	//
	//   - -1 <= x <= 1
	//   - -1 <= y <= 1
	//   -  0 <= z <= 1
	
	const bool isXInBounds = (-positionCS.w <= positionCS.x && positionCS.x <= positionCS.w);
	const bool isYInBounds = (-positionCS.w <= positionCS.y && positionCS.y <= positionCS.w);
	const bool isZInBounds = (0.0f <= positionCS.z && positionCS.z <= positionCS.w);
	
	return (isXInBounds && isYInBounds && isZInBounds);
}

struct IndexBufferWriteInfo
{
	uint IndexBufferID;
	uint MeshIndexCount;
	uint ModelInstanceID;
	uint MeshDescriptorBufferID;
	uint StartingOutputIndex;
};

void OutputIndexBufferContents(in const IndexBufferWriteInfo writeInfo)
{
	Buffer<uint> srcIndexBuffer = BrawlerHLSL::Bindless::GetGlobalUInt32Buffer(writeInfo.IndexBufferID);
	const uint packedDescriptorBufferIndices = WaveReadLaneFirst((writeInfo.ModelInstanceID << 16) | writeInfo.MeshDescriptorBufferID);
	
	const uint activeLaneIndex = WavePrefixCountBits(true);
	const uint numActiveLanes = WaveActiveCountBits(true);
	
	[loop]
	for (uint i = activeLaneIndex; i < writeInfo.MeshIndexCount; i += numActiveLanes)
	{
		const uint currLaneOutputIndex = (writeInfo.StartingOutputIndex + i);
		const uint globalVertexBufferIndex = srcIndexBuffer[NonUniformResourceIndex(i)];
		
		OutputGlobalVBIndexBuffer[NonUniformResourceIndex(currLaneOutputIndex)] = globalVertexBufferIndex;
		OutputDescriptorBufferIndicesBuffer[NonUniformResourceIndex(currLaneOutputIndex)] = packedDescriptorBufferIndices;
	}
}

struct TriangleWriteInfo
{
	BrawlerHLSL::MeshDescriptor MeshDescriptor;
	uint ModelInstanceID;
	uint MeshDescriptorBufferID;
	bool ShouldMeshBeDrawn;
};

IndexBufferWriteInfo CreateUniformIndexBufferWriteInfo(in const uint relevantLaneID, in const IndexBufferWriteInfo currLaneWriteInfo)
{
	IndexBufferWriteInfo uniformWriteInfo;
	uniformWriteInfo.IndexBufferID = WaveReadLaneAt(currLaneWriteInfo.IndexBufferID, relevantLaneID);
	uniformWriteInfo.MeshIndexCount = WaveReadLaneAt(currLaneWriteInfo.MeshIndexCount, relevantLaneID);
	uniformWriteInfo.ModelInstanceID = WaveReadLaneAt(currLaneWriteInfo.ModelInstanceID, relevantLaneID);
	uniformWriteInfo.MeshDescriptorBufferID = WaveReadLaneAt(currLaneWriteInfo.MeshDescriptorBufferID, relevantLaneID);
	uniformWriteInfo.StartingOutputIndex = WaveReadLaneAt(currLaneWriteInfo.StartingOutputIndex, relevantLaneID);
	
	return uniformWriteInfo;
}

void WriteMeshTriangles(in const TriangleWriteInfo writeInfo)
{
	// Avoid the undefined behavior incurred by accessing a descriptor table out-of-bounds.
	const uint indexBufferID = (writeInfo.ShouldMeshBeDrawn ? writeInfo.MeshDescriptor.IndexBufferSRVIndex : 0);
	Buffer<uint> meshIndexBuffer = BrawlerHLSL::Bindless::GetGlobalUInt32Buffer(indexBufferID);
	
	uint indexBufferSizeInBytes = 0;
	meshIndexBuffer.GetDimensions(indexBufferSizeInBytes);
	
	const uint currLaneIndexCount = (writeInfo.ShouldMeshBeDrawn ? (indexBufferSizeInBytes / 4) : 0);
	
	// Count the number of indices we would be writing out to the output buffers.
	const uint currWaveIndexCount = WaveActiveSum(currLaneIndexCount);
	
	uint startIndexForWave = 0;
	
	[branch]
	if (WaveIsFirstLane())
		OutputIndirectArgumentsBuffer.InterlockedAdd(0, currWaveIndexCount, startIndexForWave);
	
	IndexBufferWriteInfo indexBufferWriteInfo;
	indexBufferWriteInfo.IndexBufferID = indexBufferID;
	indexBufferWriteInfo.MeshIndexCount = currLaneIndexCount;
	indexBufferWriteInfo.ModelInstanceID = writeInfo.ModelInstanceID;
	indexBufferWriteInfo.MeshDescriptorBufferID = writeInfo.MeshDescriptorBufferID;
	indexBufferWriteInfo.StartingOutputIndex = startIndexForWave;
	
	// Do a ballot to see which lanes, if any, in the current wave have a mesh whose
	// triangles need to be written out.
	uint4 lanesWithDrawnMeshesBallot = WaveActiveBallot(writeInfo.ShouldMeshBeDrawn);
	
	while (lanesWithDrawnMeshesBallot.w != 0)
	{
		const uint relevantLaneID = WaveReadLaneFirst(firstbitlow(lanesWithDrawnMeshesBallot.w));
		
		const IndexBufferWriteInfo relevantIBWriteInfo = CreateUniformIndexBufferWriteInfo(relevantLaneID, indexBufferWriteInfo);
		OutputIndexBufferContents(relevantIBWriteInfo);
		
		indexBufferWriteInfo.StartingOutputIndex += relevantIBWriteInfo.MeshIndexCount;
		lanesWithDrawnMeshesBallot.w &= ~(1u << relevantLaneID);
	}
	
	while (lanesWithDrawnMeshesBallot.z != 0)
	{
		const uint relevantSubsetID = WaveReadLaneFirst(firstbitlow(lanesWithDrawnMeshesBallot.z));
		const uint relevantLaneID = (relevantSubsetID + 32);
		
		const IndexBufferWriteInfo relevantIBWriteInfo = CreateUniformIndexBufferWriteInfo(relevantLaneID, indexBufferWriteInfo);
		OutputIndexBufferContents(relevantIBWriteInfo);
		
		indexBufferWriteInfo.StartingOutputIndex += relevantIBWriteInfo.MeshIndexCount;
		lanesWithDrawnMeshesBallot.z &= ~(1u << relevantSubsetID);
	}
	
	while (lanesWithDrawnMeshesBallot.y != 0)
	{
		const uint relevantSubsetID = WaveReadLaneFirst(firstbitlow(lanesWithDrawnMeshesBallot.y));
		const uint relevantLaneID = (relevantSubsetID + 64);
		
		const IndexBufferWriteInfo relevantIBWriteInfo = CreateUniformIndexBufferWriteInfo(relevantLaneID, indexBufferWriteInfo);
		OutputIndexBufferContents(relevantIBWriteInfo);
		
		indexBufferWriteInfo.StartingOutputIndex += relevantIBWriteInfo.MeshIndexCount;
		lanesWithDrawnMeshesBallot.y &= ~(1u << relevantSubsetID);
	}
	
	while (lanesWithDrawnMeshesBallot.x != 0)
	{
		const uint relevantSubsetID = WaveReadLaneFirst(firstbitlow(lanesWithDrawnMeshesBallot.x));
		const uint relevantLaneID = (relevantSubsetID + 96);
		
		const IndexBufferWriteInfo relevantIBWriteInfo = CreateUniformIndexBufferWriteInfo(relevantLaneID, indexBufferWriteInfo);
		OutputIndexBufferContents(relevantIBWriteInfo);
		
		indexBufferWriteInfo.StartingOutputIndex += relevantIBWriteInfo.MeshIndexCount;
		lanesWithDrawnMeshesBallot.x &= ~(1u << relevantSubsetID);
	}
}

// Thread Contract:
//
// Create one thread group per model instance, up to the maximum number of supported model instances
// (i.e., BrawlerHLSL::GPUSceneLimits::MAX_MODEL_INSTANCES).
[numthreads(NUM_THREADS_X, 1, 1)]
void main(in const ThreadInfo threadInfo)
{
	[branch]
	if (threadInfo.DispatchThreadID == 0)
	{
		// One thread should be responsible for setting InstanceCount to one. This should only
		// cause divergence in exactly one wave.
		OutputIndirectArgumentsBuffer.Store(4, 1);
	}
	
	// We create one thread group per model instance.
	const uint modelInstanceID = threadInfo.GroupID;
	BrawlerHLSL::ModelInstanceDescriptor modelInstanceDescriptor = BrawlerHLSL::Bindless::GetGlobalModelInstanceDescriptor(modelInstanceID);
	
	// Avoid the undefined behavior incurred by accessing a descriptor table out-of-bounds.
	modelInstanceDescriptor.TransformDataBufferIndex = (modelInstanceDescriptor.IsValid ? modelInstanceDescriptor.TransformDataBufferIndex : 0);
	modelInstanceDescriptor.MeshDescriptorBufferID = (modelInstanceDescriptor.IsValid ? modelInstanceDescriptor.MeshDescriptorBufferID : 0);
	
	const BrawlerHLSL::ModelInstanceTransformData modelInstanceTransformData = BrawlerHLSL::Bindless::GetGlobalModelInstanceTransformData(modelInstanceDescriptor.TransformDataBufferIndex);
	
	const float4x4 worldMatrix = Util::Transform::ExpandWorldMatrix(modelInstanceTransformData.CurrentFrameWorldMatrix);
	
	const BrawlerHLSL::ViewDescriptor viewDescriptor = BrawlerHLSL::Bindless::GetGlobalViewDescriptor(ModelInstanceFrustumCullConstants.ViewID);
	const float4x4 viewProjectionMatrix = WaveReadLaneFirst(BrawlerHLSL::Bindless::GetGlobalViewTransformData(viewDescriptor.ViewTransformBufferIndex).CurrentFrameViewProjectionMatrix);
	
	const float4x4 worldViewProjectionMatrix = mul(worldMatrix, viewProjectionMatrix);
	
	// Each lane within a thread group will be responsible for culling the AABB of a single
	// mesh belonging to a model instance against the view frustum.
	StructuredBuffer<BrawlerHLSL::MeshDescriptor> meshDescriptorBuffer = BrawlerHLSL::Bindless::GetGlobalMeshDescriptorBuffer(modelInstanceDescriptor.MeshDescriptorBufferID);
	
	uint numMeshes = 0;
	uint structureStride = 0;
	
	meshDescriptorBuffer.GetDimensions(numMeshes, structureStride);
	
	// Don't bother checking this model instance if it isn't valid.
	const uint numIterations = (modelInstanceDescriptor.IsValid ? (numMeshes / NUM_THREADS_IN_THREAD_GROUP) + min(numMeshes % NUM_THREADS_IN_THREAD_GROUP, 1) : 0);
	
	for (uint i = 0; i < numIterations; ++i)
	{
		const uint currLaneMeshID = ((NUM_THREADS_IN_THREAD_GROUP * i) + threadInfo.GroupIndex);
		const bool isCurrLaneUseful = (currLaneMeshID < numMeshes);
		
		const BrawlerHLSL::MeshDescriptor currLaneMeshDescriptor = meshDescriptorBuffer[NonUniformResourceIndex(currLaneMeshID)];

		const float3 aabbCenter = ((currLaneMeshDescriptor.CurrentFrameAABBMin + currLaneMeshDescriptor.CurrentFrameAABBMax) * 0.5f);
		const float3 aabbExtent = (currLaneMeshDescriptor.CurrentFrameAABBMax - aabbCenter);
		
		float4 transformedAABBPointArr[8];
		transformedAABBPointArr[0] = mul(float4(currLaneMeshDescriptor.CurrentFrameAABBMax, 1.0f), worldViewProjectionMatrix); // aabbCenter + float3(aabbExtent.x, aabbExtent.y, aabbExtent.z)
		transformedAABBPointArr[1] = mul(float4(aabbCenter + float3(aabbExtent.x, aabbExtent.y, -aabbExtent.z), 1.0f), worldViewProjectionMatrix);
		transformedAABBPointArr[2] = mul(float4(aabbCenter + float3(aabbExtent.x, -aabbExtent.y, aabbExtent.z), 1.0f), worldViewProjectionMatrix);
		transformedAABBPointArr[3] = mul(float4(aabbCenter + float3(aabbExtent.x, -aabbExtent.y, -aabbExtent.z), 1.0f), worldViewProjectionMatrix);
		transformedAABBPointArr[4] = mul(float4(aabbCenter + float3(-aabbExtent.x, aabbExtent.y, aabbExtent.z), 1.0f), worldViewProjectionMatrix);
		transformedAABBPointArr[5] = mul(float4(aabbCenter + float3(-aabbExtent.x, aabbExtent.y, -aabbExtent.z), 1.0f), worldViewProjectionMatrix);
		transformedAABBPointArr[6] = mul(float4(aabbCenter + float3(-aabbExtent.x, -aabbExtent.y, aabbExtent.z), 1.0f), worldViewProjectionMatrix);
		transformedAABBPointArr[7] = mul(float4(currLaneMeshDescriptor.CurrentFrameAABBMin, 1.0f), worldViewProjectionMatrix); // aabbCenter + float3(-aabbExtent.x, -aabbExtent.y, -aabbExtent.z)
		
		// The mesh for this model instance is within the view frustum iff there exists at
		// least one corner point of the transformed AABB which lies within it.
		bool isMeshInViewFrustum = false;
		
		[unroll]
		for (uint pointIndex = 0; pointIndex < 8; ++pointIndex)
			isMeshInViewFrustum = (isMeshInViewFrustum || IsPointWithinViewFrustum(transformedAABBPointArr[pointIndex]));

		TriangleWriteInfo currLaneWriteInfo;
		currLaneWriteInfo.MeshDescriptor = currLaneMeshDescriptor;
		currLaneWriteInfo.ModelInstanceID = modelInstanceID;
		currLaneWriteInfo.MeshDescriptorBufferID = currLaneMeshID;
		currLaneWriteInfo.ShouldMeshBeDrawn = (isCurrLaneUseful && isMeshInViewFrustum);
		
		WriteMeshTriangles(currLaneWriteInfo);
	}
}