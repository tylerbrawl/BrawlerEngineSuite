#include "BindlessDescriptors.hlsli"

struct FrustumCullConstants
{
	uint ViewID;
	uint NumThreadsDispatched;
};

ConstantBuffer<FrustumCullConstants> ConstantsInfo : register(b0, space0);
ByteAddressBuffer ModelInstanceIndexBuffer : register(t0, space0);
	
struct GPUJobInfo
{
	uint ModelInstanceID;
	uint StartingTriangleClusterIDForJob;
	uint NumTriangleClustersForJob;
	uint __Pad0;
};
	
// This is an attempted port of Brawler::ThreadSafeQueue to HLSL. The idea for this
// was inspired by the UE5 Nanite presentation, where they also used a multi-producer,
// multi-consumer queue to do culling on the GPU without creating additional dispatches.
// However, implementation details for that were largely left uncovered (at least in the
// Advances in Real-Time Rendering presentation).
//
// As it turns out, the bounded ring buffer queue used in the CPU code is probably one
// of the only (efficient?) ways to actually implement a multi-producer, multi-consumer
// queue in HLSL, since we don't have dynamic memory allocations in shaders.
RWStructuredBuffer<GPUJobInfo> GPUJobQueue : register(u0, space0);
RWBuffer<uint> GPUJobQueueIndexBuffer : register(u1, space0);
	
RWBuffer<uint> ProcessedModelInstanceCount : register(u2, space0);
	
struct ModelInstanceProcessingInfo
{
	uint ModelInstanceID;
	uint NumModelInstances;
	bool IsModelInstanceDataUseful;
};
	
void AddTriangleClustersForModelInstance(in const ModelInstanceProcessingInfo processingInfo)
{
	
}
	
[numthreads(64, 1, 1)]
void main(in const uint DTid : SV_DispatchThreadID)
{
	const uint modelInstanceCount = WaveReadLaneFirst(ModelInstanceIndexBuffer.Load(0));
		
	// Determine whether or not this lane's model instance data is actually valid. To reduce
	// divergence within a wave, lanes which do not have useful model instance data will still
	// take the code path for model instance culling if at least one lane in the wave does
	// have useful data.
	const bool isModelInstanceDataUseful = (DTid <= modelInstanceCount);
		
	// [Coherent]
	[branch]
	if (WaveActiveAnyTrue(isModelInstanceDataUseful))
}