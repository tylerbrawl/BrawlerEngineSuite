#include "GPUSceneLimits.hlsli"
#include "MeshConstants.hlsli"

namespace BrawlerHLSL
{
	struct PackedTriangleCluster
	{
		float3 CurrentFrameAABBMin;
		uint MaterialDefinitionIndex : 25;
		uint BiasedTriangleCount : 7;

		float3 CurrentFrameAABBMax;
		uint StartingIndexBufferIndex;
	};
	
	struct UnpackedTriangleCluster
	{
		float3 CurrentFrameAABBMin;
		uint MaterialDefinitionIndex;
		float3 CurrentFrameAABBMax;
		uint TriangleCount;
		uint StartingIndexBufferIndex;
	};
	
	PackedTriangleCluster PackTriangleCluster(in const UnpackedTriangleCluster unpackedCluster)
	{
		PackedTriangleCluster packedCluster;
		packedCluster.CurrentFrameAABBMin = unpackedCluster.CurrentFrameAABBMin;
		packedCluster.MaterialDefinitionIndex = unpackedCluster.MaterialDefinitionIndex;
		
		// Similar to how floats store a biased exponent, we store a biased triangle count
		// in the PackedTriangleCluster. Essentially, we store (TriangleCount - 1) with the
		// assumption that no TriangleCluster has 0 triangles. This allows us to count 128
		// triangles with 7 bits, rather than 8.
		packedCluster.BiasedTriangleCount = (unpackedCluster.TriangleCount - 1);

		packedCluster.CurrentFrameAABBMax = unpackedCluster.CurrentFrameAABBMax;
		packedCluster.StartingIndexBufferIndex = unpackedCluster.StartingIndexBufferIndex;
		
		return packedCluster;
	}
	
	UnpackedTriangleCluster UnpackTriangleCluster(in const PackedTriangleCluster packedCluster)
	{
		UnpackedTriangleCluster unpackedCluster;
		unpackedCluster.CurrentFrameAABBMin = packedCluster.CurrentFrameAABBMin;
		unpackedCluster.MaterialDefinitionIndex = packedCluster.MaterialDefinitionIndex;
		unpackedCluster.CurrentFrameAABBMax = packedCluster.CurrentFrameAABBMax;
		unpackedCluster.TriangleCount = (packedCluster.BiasedTriangleCount + 1);
		unpackedCluster.StartingIndexBufferIndex = packedCluster.StartingIndexBufferIndex;
		
		return unpackedCluster;
	}
}
