#include "BindlessDescriptors.hlsli"

struct PrepareConstants
{
	uint ViewID;
};

ConstantBuffer<PrepareConstants> ConstantsInfo : register(b0, space0);
RWByteAddressBuffer ModelInstanceIndexOutputBuffer : register(u0, space0);
	
bool IsTransformedPointWithinFrustum(in const float4 transformedPoint)
{
	return (transformedPoint.x >= -transformedPoint.w && transformedPoint.x <= transformedPoint.w && transformedPoint.y >= -transformedPoint.w &&
		transformedPoint.y <= transformedPoint.w && transformedPoint.z >= 0.0f && transformedPoint.z <= transformedPoint.w);
}
	
bool IsModelInstanceAABBWithinViewFrustum(in const BrawlerHLSL::ModelInstanceData instanceData)
{
	// Transform all of the vertices of the AABB into homogeneous clip space. If all of
	// the points lie outside of at least one boundary, then the AABB is *NOT* intersecting
	// or inside of the view frustum.
		
	const float4x4 worldViewProjMatrix = (instanceData.TransformData.CurrentFrameWorldMatrix * BrawlerHLSL::Bindless::GetGlobalViewTransformData(ConstantsInfo.ViewID).CurrentFrameViewProjectionMatrix);
		
	uint outsidePlaneMask = 0xFFFFFFFF;
	const float4 aabbMaxPoint = float4(instanceData.LODMeshData.CurrentFrameAABBMax, 1.0f);
	const float4 aabbMinPoint = float4(instanceData.LODMeshData.CurrentFrameAABBMin, 1.0f);
		
	float4 aabbPointArr[8];
	aabbPointArr[0] = float4(aabbMinPoint.x, aabbMinPoint.y, aabbMinPoint.z, 1.0f) * worldViewProjMatrix;
	aabbPointArr[1] = float4(aabbMinPoint.x, aabbMinPoint.y, aabbMaxPoint.z, 1.0f) * worldViewProjMatrix;
	aabbPointArr[2] = float4(aabbMinPoint.x, aabbMaxPoint.y, aabbMinPoint.z, 1.0f) * worldViewProjMatrix;
	aabbPointArr[3] = float4(aabbMinPoint.x, aabbMaxPoint.y, aabbMaxPoint.z, 1.0f) * worldViewProjMatrix;
	aabbPointArr[4] = float4(aabbMaxPoint.x, aabbMinPoint.y, aabbMinPoint.z, 1.0f) * worldViewProjMatrix;
	aabbPointArr[5] = float4(aabbMaxPoint.x, aabbMinPoint.y, aabbMaxPoint.z, 1.0f) * worldViewProjMatrix;
	aabbPointArr[6] = float4(aabbMaxPoint.x, aabbMaxPoint.y, aabbMinPoint.z, 1.0f) * worldViewProjMatrix;
	aabbPointArr[7] = float4(aabbMaxPoint.x, aabbMaxPoint.y, aabbMaxPoint.z, 1.0f) * worldViewProjMatrix;
		
	bool isAABBWithinFrustum = false;
		
	[unroll]
	for (uint i = 0; i < 8; ++i)
		isAABBWithinFrustum = (isAABBWithinFrustum || IsTransformedPointWithinFrustum(aabbPointArr[i]));
		
	return isAABBWithinFrustum;
}

[numthreads(64, 1, 1)]
void main(in const uint DTid : SV_DispatchThreadID)
{
	const BrawlerHLSL::ModelInstanceLODMeshData currLaneLODMeshData = BrawlerHLSL::Bindless::GetGlobalModelInstanceLODMeshData(DTid);
		
	// A model instance which is actually valid should have at least one triangle cluster.
	bool isModelInstanceInUse = (currLaneLODMeshData.NumTriangleClusters > 0);
	
	// Get outta here if the entire wave is useless!
	[branch]
	if (WaveActiveAllTrue(!isModelInstanceInUse))
		return;
		
	// Perform frustum culling for the bounding box of this model instance.
	BrawlerHLSL::ModelInstanceData currLaneInstanceData;
	currLaneInstanceData.TransformData = BrawlerHLSL::Bindless::GetGlobalModelInstanceTransformData(DTid);
	currLaneInstanceData.LODMeshData = currLaneLODMeshData;
		
	isModelInstanceInUse = (isModelInstanceInUse && IsModelInstanceAABBWithinViewFrustum(currLaneInstanceData));
		
	const uint numValidModelInstancesInWave = WaveActiveCountBits(isModelInstanceInUse);
	uint waveOffsetFromBufferStart;
		
	[branch]
	if (WaveIsFirstLane())
	{
		// Add four to the offset so that we do not accidentally overwrite the "counter" value.
		//
		// (I used quotes around the term "counter" because it's not actually a UAV counter.)
		waveOffsetFromBufferStart = (ModelInstanceIndexOutputBuffer.InterlockedAdd(0, numValidModelInstancesInWave, waveOffsetFromBufferStart) + 4);
	}
	
	const uint currLaneOffsetFromOutputBufferStart = WaveReadLaneFirst(waveOffsetFromBufferStart) + (4 * WavePrefixCountBits(isModelInstanceInUse));

	// If the model instance selected by this lane is both valid and within the view frustum, then 
	// add its ID to the lane's designated slot in ModelInstanceIndexOutputBuffer.
	[branch]
	if (isModelInstanceInUse)
		ModelInstanceIndexOutputBuffer.Store(currLaneOffsetFromOutputBufferStart, DTid);
}