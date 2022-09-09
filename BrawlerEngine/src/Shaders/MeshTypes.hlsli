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
		float4x3 CurrentFrameWorldMatrix;
		float4x3 CurrentFrameInverseWorldMatrix;
		
		float4x3 PreviousFrameWorldMatrix;
		float4x3 PreviousFrameInverseWorldMatrix;
		
		// In addition, the following matrices can be calculated in a shader, should
		// they prove necessary:
		//
		//   - InverseTransposeWorldMatrix = transpose(InverseWorldMatrix)  // This works because (Inverse(Transpose(M)) == Transpose(Inverse(M))) for any invertible matrix M.
	};

	struct LODMeshData
	{
		float3 CurrentFrameAABBMin;
		uint StartingTriangleClusterID;
		
		float3 CurrentFrameAABBMax;
		uint NumTriangleClusters;
	};
	
	struct ModelInstanceData
	{
		ModelInstanceTransformData TransformData;
		LODMeshData LODMesh;
	};
	
	struct ModelInstanceDescriptor
	{
		uint RenderDataBufferIndex;
		uint IsUseful : 1;
		uint __PaddingBits : 31;
		uint2 __Pad0;
	};
}