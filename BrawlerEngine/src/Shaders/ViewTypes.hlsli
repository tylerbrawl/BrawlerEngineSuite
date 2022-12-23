#pragma once

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
		/// This is the origin of the view space coordinate system with respect
		/// to the world space coordinate system for the current frame. 
		/// 
		/// This value can be used to get the view's position in world space.
		/// To use it in the construction of a view matrix, however, this value
		/// must be negated.
		/// </summary>
		float3 CurrentFrameViewOriginWS;
		
		uint __Pad0;
		
		float4 PreviousFrameViewSpaceQuaternion;
		
		/// <summary>
		/// This is the origin of the view space coordinate system with respect
		/// to the world space coordinate system for the previous frame. 
		/// 
		/// This value can be used to get the view's position in world space.
		/// To use it in the construction of a view matrix, however, this value
		/// must be negated.
		/// </summary>
		float3 PreviousFrameViewOriginWS;
		
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