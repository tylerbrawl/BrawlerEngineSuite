namespace BrawlerHLSL
{
	// We might also be able to use this exact same struct for skinned data, too! The difference there
	// is that we would be creating a separate buffer containing the bone indices and weights.
	struct PackedStaticVertex
	{
		float4 PositionAndTangentFrame;
	
		float2 UVCoords;
		uint2 __Pad0;
	};

	// NOTE: *ALL* matrices are made implicitly row-major by the Brawler Shader Compiler.
	
	struct ModelInstanceTransformData
	{
		float4x4 CurrentFrameWorldMatrix;
		float4x4 PreviousFrameWorldMatrix;
	};

	struct ModelInstanceLODMeshData
	{
		float3 CurrentFrameAABBMin;
		uint StartingTriangleClusterID;
		
		float3 CurrentFrameAABBMax;
		uint NumTriangleClusters;
	};
	
	struct ModelInstanceData
	{
		ModelInstanceTransformData TransformData;
		ModelInstanceLODMeshData LODMeshData;
	};
}