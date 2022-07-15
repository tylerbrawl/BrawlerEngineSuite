namespace BrawlerHLSL
{
	struct ViewTransformData
	{
		float4x4 CurrentFrameViewProjectionMatrix;
		float4x4 CurrentFrameInverseViewProjectionMatrix;
		
		float4x4 PreviousFrameViewProjectionMatrix;
		float4x4 PreviousFrameInverseViewProjectionMatrix;
		
		float4 CurrentFrameViewSpaceQuaternion;
		float4 PreviousFrameViewSpaceQuaternion;
		
		// In addition, the following matrices can be calculated in a shader, should
		// they prove necessary:
		//
		//   - InverseProjection = InverseViewProjection * View
		//   - InverseView = transpose(View)  // This works because the view matrix is an orthogonal matrix, and for any orthogonal matrix M, (Inverse(M) == Transpose(M)).
	};

	struct ViewDimensionsData
	{
		float2 ViewDimensions;
		float2 InverseViewDimensions;
	};
}