namespace BrawlerHLSL
{
	struct ViewTransformData
	{
		float4x4 CurrentFrameViewMatrix;
		float4x4 CurrentFrameViewProjectionMatrix;
		
		float4x4 PreviousFrameViewMatrix;
		float4x4 PreviousFrameViewProjectionMatrix;
	};

	struct ViewDimensionsData
	{
		float2 ViewDimensions;
		float2 InverseViewDimensions;
	};
}