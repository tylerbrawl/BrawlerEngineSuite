namespace BrawlerHLSL
{
	struct ViewTransformData
	{
		float3x3 CurrentFrameViewMatrix;
		float4x4 CurrentFrameViewProjectionMatrix;
		float4x4 CurrentFrameInverseViewProjectionMatrix;
		
		float3x3 PreviousFrameViewMatrix;
		float4x4 PreviousFrameViewProjectionMatrix;
		float4x4 PreviousFrameInverseViewProjectionMatrix;
		
		// In addition, the following matrices can be calculated in a shader, should
		// they prove necessary:
		//
		//   - InverseProjection = InverseViewProjection * View
	};

	struct ViewDimensionsData
	{
		float2 ViewDimensions;
		float2 InverseViewDimensions;
	};
}