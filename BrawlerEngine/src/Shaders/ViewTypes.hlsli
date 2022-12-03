namespace BrawlerHLSL
{
	struct ViewTransformData
	{
		float4x4 CurrentFrameViewProjectionMatrix;
		float4x4 CurrentFrameInverseViewProjectionMatrix;
		
		float4x4 PreviousFrameViewProjectionMatrix;
		float4x4 PreviousFrameInverseViewProjectionMatrix;
		
		float4 CurrentFrameViewSpaceQuaternion;
		
		/// <summary>
		/// This is the origin of the world space coordinate system with respect
		/// to the view space coordinate system for the current frame. 
		/// 
		/// It is done like this so that the values can directly be used in the 
		/// construction of a view space matrix.
		/// </summary>
		float3 CurrentFrameWorldSpaceOriginVS;
		
		uint __Pad0;
		
		float4 PreviousFrameViewSpaceQuaternion;
		
		/// <summary>
		/// This is the origin of the world space coordinate system with respect
		/// to the view space coordinate system for the previous frame. 
		/// 
		/// It is done like this so that the values can directly be used in the 
		/// construction of a view space matrix.
		/// </summary>
		float3 PreviousFrameWorldSpaceOriginVS;
		
		uint __Pad1;
		
		// In addition, the following matrices can be calculated in a shader, should
		// they prove necessary:
		//
		//   - InverseProjection = InverseViewProjection * View
		//   - InverseView = transpose(View)  // This works because the view matrix is an orthogonal matrix, and for any orthogonal matrix M, (Inverse(M) == Transpose(M)).
	};

	struct ViewDimensionsData
	{
		uint2 ViewDimensions;
		float2 InverseViewDimensions;
	};
	
	struct PackedViewDescriptor
	{
		uint ViewTransformBufferIndex : 12;
		uint ViewDimensionsBufferIndex : 12;
		uint __Pad0 : 8;
	};
	
	struct ViewDescriptor
	{
		uint ViewTransformBufferIndex;
		uint ViewDimensionsBufferIndex;
	};
}