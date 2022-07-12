#include "BindlessDescriptors.hlsli"
#include "Optional.hlsli"

struct PrepareConstants
{
	uint ViewID;
	uint NumThreadsDispatched;
		
	// Even though this value can be computed in the shader as (NumThreadsDispatched / WaveGetLaneCount()),
	// we make it an explicit root constant because the value is uniform across all
	// waves.
	uint NumWavesDispatched;
		
	// This value should be the number of elements supported by the queue's underlying
	// buffer plus one. 
	// 
	// Specifically, let X be the desired number of queue elements (that is, the CPU-side 
	// equivalent would be Brawler::ThreadSafeQueue<ElementType, X>). Then, NumQueueElementsPlusOne
	// should be (X + 1), while the buffers representing the GPU job queue should be sized
	// as follows:
	//
	//   - TriangleClusterGPUJobQueue: 2(X + 1) + 1 = (2X + 3) 32-bit Integers
	//   - GPUJobQueueAcknowledgementBuffer: (X + 1) + 1 = (X + 2) 32-bit Integers
	//
	// These values differ from the sizes of the underlying std::array instances used to
	// implement Brawler::ThreadSafeQueue.
	uint NumQueueElementsPlusOne;
};

ConstantBuffer<PrepareConstants> ConstantsInfo : register(b0, space0);
	
// This is an attempted port of Brawler::ThreadSafeQueue to HLSL. Although that queue was
// made for the CPU, it was implemented as a bounded ring-buffer with atomics; this makes
// it possible to re-implement it in a shader, particularly in comparison to most other
// lock-free data structures, which tend to require many dynamic memory allocations.
//
// The idea of using a multi-producer, multi-consumer (MPMC) queue in a shader comes from
// the excellent UE5 Nanite presentation given at Advances in Real-Time Rendering 2021.
// However, implementation details for such a queue in a shader were never given. Thus, the
// implementation presented here may very well be quite different from the one used in the
// Unreal Engine. Nevertheless, the idea has been battle-tested in a variety of highly
// multi-threaded scenarios on the CPU.
//
// (To Brian Karis: Thank you so much for the awesome and detailed presentation! I would love
// it if you or anyone else could offer any suggestions or improvements to this approach.)
	
// NOTE: Both buffers *MUST* be cleared to zero prior to the beginning of the dispatch on the 
// GPU timeline.
globallycoherent RWByteAddressBuffer TriangleClusterGPUJobQueue : register(u0, space0);
globallycoherent RWByteAddressBuffer GPUJobQueueAcknowledgementBuffer : register(u1, space0);
	
RWByteAddressBuffer PerViewTriangleClusterOutputBuffer : register(u2, space0);
	
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
	
struct ProcessedModelInstance
{
	BrawlerHLSL::ModelInstanceData InstanceData;
	uint ModelInstanceID;
	bool IsModelInstanceUseful;
};
	
struct GPUJobQueueTriangleClusterBatch
{
	uint ModelInstanceID;
	uint StartingTriangleClusterIDForBatch;
};
	
struct UnpackedQueueIndices
{
	uint BeginIndex;
	uint EndIndex;
};
	
static const uint INVALID_MODEL_INSTANCE_ID_VALUE = uint(-1);
	
namespace GPUJobQueue
{
	uint PackQueueIndices(in const UnpackedQueueIndices unpackedIndices)
	{
		return uint((unpackedIndices.BeginIndex << 16) | unpackedIndices.EndIndex);
	}
	
	UnpackedQueueIndices UnpackQueueIndices(in const uint packedIndices)
	{
		UnpackedQueueIndices unpackedIndices;
		unpackedIndices.BeginIndex = (packedIndices >> 16);
		unpackedIndices.EndIndex = (packedIndices & 0xFFFF);
		
		return unpackedIndices;
	}
		
	bool TryAddBatchToQueue(in const GPUJobQueueTriangleClusterBatch clusterBatch)
	{
		uint claimedIndex;
			
		[allow_uav_condition]
		while (true)
		{
			uint packedIndices;
		
			// Limit the load to the first wave in order to reduce the number of atomic
			// operations.
			[branch]
			if (WaveIsFirstLane())
				packedIndices = TriangleClusterGPUJobQueue.Load(0);
		
			const uint expectedStoredValue = WaveReadLaneFirst(packedIndices);
				
			UnpackedQueueIndices unpackedIndices = UnpackQueueIndices(expectedStoredValue);
				
			unpackedIndices.EndIndex = ((unpackedIndices.EndIndex + 1) % ConstantsInfo.NumQueueElementsPlusOne);
			claimedIndex = unpackedIndices.EndIndex;
				
			// Rather than creating a buffer/array with N elements, we create a buffer
			// with (N + 1) elements. That way, we can check to see if the queue is "full"
			// by checking if after the addition of the element, the two indices would
			// become equal.
				
			// [Coherent]
			[branch]
			if (unpackedIndices.EndIndex == unpackedIndices.BeginIndex)
				return false;
				
			packedIndices = PackQueueIndices(unpackedIndices);
			uint actualStoredValue;
				
			[branch]
			if (WaveIsFirstLane())
			{
				TriangleClusterGPUJobQueue.InterlockedCompareExchange(
					0,
					expectedStoredValue,
					packedIndices,
					actualStoredValue
				);
			}
				
			actualStoredValue = WaveReadLaneFirst(actualStoredValue);
				
			// Exit the loop if this wave passed the compare-exchange test.
				
			// [Coherent]
			[branch]
			if (actualStoredValue == expectedStoredValue)
				break;
		}
			
		// We successfully reserved an index within the queue for this wave. We can now write the
		// value into the queue.
		//
		// Unlike C++, HLSL has no std::memory_order semantics, but we do have raw memory barriers.
		// We need to make sure that the writes to the job queue buffer are always done before the
		// write to the acknowledgement buffer.
			
		const uint2 rawQueueEntryValue = uint2(clusterBatch.ModelInstanceID, clusterBatch.StartingTriangleClusterIDForBatch);
			
		[branch]
		if (WaveIsFirstLane())
			TriangleClusterGPUJobQueue.Store2((claimedIndex * 8) + 4, rawQueueEntryValue);
			
		// The write to GPUJobQueueAcknowledgementBuffer "synchronizes-with" the write to
		// TriangleClusterGPUJobQueue.
		DeviceMemoryBarrierWithGroupSync();
			
		[branch]
		if (WaveIsFirstLane())
			GPUJobQueueAcknowledgementBuffer.Store((claimedIndex * 4) + 4, 1);
			
		return true;
	}
		
	GPUJobQueueTriangleClusterBatch TryRemoveBatchFromQueue()
	{
		uint claimedIndex;
			
		[allow_uav_condition]
		while (true)
		{
			uint packedIndices;
				
			[branch]
			if (WaveIsFirstLane())
				packedIndices = TriangleClusterGPUJobQueue.Load(0);
				
			const uint expectedStoredValue = WaveReadLaneFirst(packedIndices);
				
			UnpackedQueueIndices unpackedIndices = UnpackQueueIndices(expectedStoredValue);
				
			// If the queue is empty, then we should return an invalid value. (I was thinking
			// of making a BrawlerHLSL::Optional<T> class, but since there is no such thing as
			// encapsulation with HLSL classes, I found it to be a moot point.)
				
			// [Coherent]
			[branch]
			if (unpackedIndices.BeginIndex == unpackedIndices.EndIndex)
			{
				GPUJobQueueTriangleClusterBatch invalidBatch;
				invalidBatch.ModelInstanceID = INVALID_MODEL_INSTANCE_ID_VALUE;
					
				return invalidBatch;
			}
				
			unpackedIndices.BeginIndex = ((unpackedIndices.BeginIndex + 1) % ConstantsInfo.NumQueueElementsPlusOne);
			claimedIndex = unpackedIndices.BeginIndex;
				
			packedIndices = PackQueueIndices(unpackedIndices);
			uint actualStoredValue;
				
			[branch]
			if (WaveIsFirstLane())
			{
				TriangleClusterGPUJobQueue.InterlockedCompareExchange(
					0,
					expectedStoredValue,
					packedIndices,
					actualStoredValue
				);
			}
				
			actualStoredValue = WaveReadLaneFirst(actualStoredValue);
				
			// Exit the loop if this wave passed the compare-exchange test.
				
			// [Coherent]
			[branch]
			if (actualStoredValue == expectedStoredValue)
				break;
		}
			
		// Do a short spinlock until a producer thread/wave notes that it has written out
		// the value which we are looking for.
		[allow_uav_condition]
		while (true)
		{
			static const uint EXPECTED_ACKNOWLEDGEMENT_BUFFER_VALUE = 1;
				
			uint actualAcknowledgementBufferValue;
				
			[branch]
			if (WaveIsFirstLane())
				GPUJobQueueAcknowledgementBuffer.InterlockedExchange((claimedIndex * 4) + 4, 0, actualAcknowledgementBufferValue);
				
			actualAcknowledgementBufferValue = WaveReadLaneFirst(actualAcknowledgementBufferValue);
				
			// Exit the loop if the value which was originally stored was a 1 (that is,
			// the value was written by a producer to signify that it has written out the required
			// value).
				
			// [Coherent]
			[branch]
			if (actualAcknowledgementBufferValue == EXPECTED_ACKNOWLEDGEMENT_BUFFER_VALUE)
				break;
		}
			
		// The read from TriangleClusterGPUJobQueue "synchronizes-with" the read/write from
		// GPUJobQueueAcknowledgementBuffer.
		DeviceMemoryBarrierWithGroupSync();
			
		uint2 rawQueueEntryValue;
			
		[branch]
		if (WaveIsFirstLane())
			rawQueueEntryValue = TriangleClusterGPUJobQueue.Load2((claimedIndex * 8) + 4);
			
		rawQueueEntryValue = WaveReadLaneFirst(rawQueueEntryValue);
			
		GPUJobQueueTriangleClusterBatch clusterBatch;
		clusterBatch.ModelInstanceID = rawQueueEntryValue.x;
		clusterBatch.StartingTriangleClusterIDForBatch = rawQueueEntryValue.y;
			
		return clusterBatch;
	}
}
	
void ProcessTriangleClusterBatch(in const GPUJobQueueTriangleClusterBatch clusterBatch)
{
	// Every batch refers to at most WaveGetLaneCount() triangle clusters. However, the actual
	// number of clusters might be less than this. Although this information isn't included
	// in clusterBatch, we can infer it by making some calculations.
		
	const BrawlerHLSL::ViewTransformData viewTransformData = BrawlerHLSL::Bindless::GetGlobalViewTransformData(ConstantsInfo.ViewID);
	const BrawlerHLSL::ViewDimensionsData viewDimensionsData = BrawlerHLSL::Bindless::GetGlobalViewDimensionsData(ConstantsInfo.ViewID);
	const BrawlerHLSL::ModelInstanceTransformData modelInstanceTransformData = BrawlerHLSL::Bindless::GetGlobalModelInstanceTransformData(clusterBatch.ModelInstanceID);
	const BrawlerHLSL::LODMeshData lodMeshDataForBatch = BrawlerHLSL::Bindless::GetGlobalModelInstanceLODMeshData(clusterBatch.ModelInstanceID);
	const uint numTriangleClustersForBatch = lodMeshDataForBatch.NumTriangleClusters - (clusterBatch.StartingTriangleClusterIDForBatch - lodMeshDataForBatch.StartingTriangleClusterID);
		
	const bool doesCurrLaneHaveTriangleCluster = (WaveGetLaneIndex() < numTriangleClustersForBatch);
	const uint currLaneTriangleClusterID = (clusterBatch.StartingTriangleClusterIDForBatch + WaveGetLaneIndex());
	const BrawlerHLSL::UnpackedTriangleCluster currLaneCluster = BrawlerHLSL::UnpackTriangleCluster(BrawlerHLSL::Bindless::GetGlobalPackedTriangleCluster(currLaneTriangleClusterID));
		
	// Perform additional frustum culling for the bounding box of each triangle cluster in
	// this batch. Since we already did complete frustum culling for each model instance's AABB,
	// it should suffice to perform the frustum culling test against the cluster's AABB in NDC
	// space. This test is described in "Optimizing the Graphics Pipeline with Compute" by Graham
	// Wihlidal.
		
	const float4x4 worldViewProjMatrix = (modelInstanceTransformData.CurrentFrameWorldMatrix * viewTransformData.CurrentFrameViewProjectionMatrix);
		
	float4 currLaneClusterAABBMin = float4(currLaneCluster.CurrentFrameAABBMin, 1.0f) * worldViewProjMatrix;
	float4 currLaneClusterAABBMax = float4(currLaneCluster.CurrentFrameAABBMax, 1.0f) * worldViewProjMatrix;
		
	currLaneClusterAABBMin.xyz /= currLaneClusterAABBMin.w;
	currLaneClusterAABBMax.xyz /= currLaneClusterAABBMax.w;
		
	const bool isClusterWithinView = (currLaneClusterAABBMin.x >= -1.0f && currLaneClusterAABBMax.x <= 1.0f && currLaneClusterAABBMin.y >= -1.0f &&
		currLaneClusterAABBMax.y <= 1.0f);
	const bool isCurrLaneUseful = (doesCurrLaneHaveTriangleCluster && isClusterWithinView);
		
	const uint numLanesWithVisibleCluster = WaveActiveCountBits(isCurrLaneUseful);
	uint baseOffsetForWave;
		
	[branch]
	if (WaveIsFirstLane())
		PerViewTriangleClusterOutputBuffer.InterlockedAdd(0, numLanesWithVisibleCluster, baseOffsetForWave);
		
	baseOffsetForWave = WaveReadLaneFirst((baseOffsetForWave * 8) + 4);
	const uint baseOffsetForLane = baseOffsetForWave + (WavePrefixCountBits(isCurrLaneUseful) * 8);
		
	[branch]
	if (isCurrLaneUseful)
		PerViewTriangleClusterOutputBuffer.Store2(baseOffsetForLane, uint2(clusterBatch.ModelInstanceID, currLaneTriangleClusterID));
}
	
template <uint LaneOffset>
struct BatchAdder
{
	static void AddBatchToQueue(in const ProcessedModelInstance modelInstance, in const uint numBatchesForCurrLane, in uint currBallot)
	{
		while (currBallot > 0)
		{
			// Explicitly scalarize the lane which we are processing.
			const uint laneToProcess = WaveReadLaneFirst(firstbitlow(currBallot) + LaneOffset);
			
			// Get both the model instance ID and the starting triangle cluster ID. In addition, since
			// we already calculated it, get the number of batches required for this model instance. The 
			// retrieved values will be uniform (scalarized) because laneToProcess is uniform (scalarized).
			GPUJobQueueTriangleClusterBatch currBatch;
			currBatch.ModelInstanceID = WaveReadLaneAt(modelInstance.ModelInstanceID, laneToProcess);
			currBatch.StartingTriangleClusterIDForBatch = WaveReadLaneAt(modelInstance.InstanceData.LODMeshData.StartingTriangleClusterID, laneToProcess);
			
			const uint numBatchesToAdd = WaveReadLaneAt(numBatchesForCurrLane, laneToProcess);
			
			for (uint i = 0; i < numBatchesToAdd; ++i)
			{
				// [Coherent]
				if (!GPUJobQueue::TryAddBatchToQueue(currBatch))
					ProcessTriangleClusterBatch(currBatch);
				
				currBatch.StartingTriangleClusterIDForBatch += WaveGetLaneCount();
			}
			
			// Cancel out the lane which we just handled.
			currBallot &= ~(1 << (laneToProcess - LaneOffset));
		}
	}
};
	
void AddModelInstanceDataToQueue(in const ProcessedModelInstance modelInstance)
{
	// Perform frustum culling for the bounding box of this model instance. Since HLSL 2021 has
	// boolean short-circuiting, we should put IsModelInstanceAABBWithinViewFrustum() first to
	// reduce divergence.
	const bool isModelInstanceRelevant = (IsModelInstanceAABBWithinViewFrustum(modelInstance.InstanceData) && modelInstance.IsModelInstanceUseful);
		
	// Get outta here if the entire wave is useless!
	[branch]
	if (WaveActiveAllTrue(!isModelInstanceRelevant))
		return;
		
	// To minimize divergence, we want to push batches of WaveGetLaneCount() triangle clusters to
	// the GPU job queue.
	const uint numTriangleClustersForCurrLane = modelInstance.InstanceData.LODMeshData.NumTriangleClusters;
	const uint numBatchesForCurrLane = ((numTriangleClustersForCurrLane / WaveGetLaneCount()) + min(numTriangleClustersForCurrLane % WaveGetLaneCount(), 1)) * (isModelInstanceRelevant ? 1 : 0);

	const uint4 lanesWithBatchesBallot = WaveActiveBallot(isModelInstanceRelevant);
		
	// In an ideal world, WaveGetLaneCount() would be consteval, and we could use that in constant
	// expressions evaluated at PSO compile time. This isn't an ideal world, however.
	BatchAdder<0>::AddBatchToQueue(modelInstance, numBatchesForCurrLane, lanesWithBatchesBallot.w);
	BatchAdder<32>::AddBatchToQueue(modelInstance, numBatchesForCurrLane, lanesWithBatchesBallot.z);
	BatchAdder<64>::AddBatchToQueue(modelInstance, numBatchesForCurrLane, lanesWithBatchesBallot.y);
	BatchAdder<96>::AddBatchToQueue(modelInstance, numBatchesForCurrLane, lanesWithBatchesBallot.x);
}

[numthreads(64, 1, 1)]
void main(in const uint DTid : SV_DispatchThreadID, in const uint groupIndex : SV_GroupIndex)
{
	// For every possible model instance, check if we have valid model instance data.
	// This might seem slow, but we create one thread for each lane which the GPU is capable
	// of running at once; that is, we create D3D12_FEATURE_DATA_D3D12_OPTIONS1::TotalLaneCount
	// lanes.
	//
	// I believe that this is what Karis meant when he stated that they created enough threads
	// for the culling queue system in UE5 Nanite to saturate the GPU. This doesn't account for 
	// occupancy, however, and since there is no way to gauge this for all of the different consumer 
	// GPUs, perhaps we should subtract some multiple from this value to get the actual number of
	// created threads. Regardless, without the ability to measure occupancy at runtime, that is
	// about as good as it will get.
	//
	// (Note, however, that the aforementioned caveat need not apply to consoles.)
		
	for (uint i = DTid; i < BrawlerHLSL::GPUSceneLimits::MAX_MODEL_INSTANCES; i += ConstantsInfo.NumThreadsDispatched)
	{
		const BrawlerHLSL::ModelInstanceLODMeshData currLaneLODMeshData = BrawlerHLSL::Bindless::GetGlobalModelInstanceLODMeshData(DTid);
		
		// A model instance which is actually valid should have at least one triangle cluster.
		bool isModelInstanceInUse = (currLaneLODMeshData.NumTriangleClusters > 0);
			
		// Don't bother if the entire wave is useless!
		[branch]
		if (WaveActiveAnyTrue(isModelInstanceInUse))
		{
			ProcessedModelInstance modelInstance;
			modelInstance.InstanceData.TransformData = BrawlerHLSL::Bindless::GetGlobalModelInstanceTransformData(DTid);
			modelInstance.InstanceData.LODMeshData = currLaneLODMeshData;
			modelInstance.ModelInstanceID = DTid;
			modelInstance.IsModelInstanceUseful = isModelInstanceInUse;
			
			AddModelInstanceDataToQueue(modelInstance);
		}
	}
	
	// Keep note of the fact that the current wave has finished processing all of its model instances.
	// Although there is no such thing as a "wave ID," we can count the number of dispatched waves and
	// determine when to exit based on that value.
	{
		uint oldFinishedWaveCount;
			
		[branch]
		if (WaveIsFirstLane())
			GPUJobQueueAcknowledgementBuffer.InterlockedAdd(0, 1, oldFinishedWaveCount);
	}
	
	// Even though this wave has finished processing its model instances, it can still process any
	// triangle clusters pushed into the GPU job queue by other waves.
	while(true)
	{
		// The value returned by GPUJobQueue::TryRemoveBatchFromQueue() will be uniform (scalarized)
		// across the entire wave.
		const GPUJobQueueTriangleClusterBatch extractedBatch = GPUJobQueue::TryRemoveBatchFromQueue();
			
		// [Coherent]
		[branch]
		if (extractedBatch.ModelInstanceID != INVALID_MODEL_INSTANCE_ID_VALUE)
			ProcessTriangleClusterBatch(extractedBatch);
		else
		{
			// If we could not extract a triangle cluster batch from the GPU job queue, then we check if
			// every wave has already processed all of its model instances. If so, then we conclude that
			// the queue has been emptied and will not receive any more entries; thus, we can leave.
			uint numWavesFinishedWithModelInstances;
				
			[branch]
			if(WaveIsFirstLane())
				numWavesFinishedWithModelInstances = GPUJobQueueAcknowledgementBuffer.Load(0);
				
			numWavesFinishedWithModelInstances = WaveReadLaneFirst(numWavesFinishedWithModelInstances);
				
			// [Coherent]
			[branch]
			if (numWavesFinishedWithModelInstances == ConstantsInfo.NumWavesDispatched)
				return;
		}
	}
}