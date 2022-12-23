#pragma once
#include "TextureCubeFace.hlsli"

namespace Util
{
	namespace Transform
	{
		inline float4x4 ExpandWorldMatrix(in const float4x3 worldMatrix)
		{
			return float4x4(
				float4(worldMatrix[0].xyz, 0.0f),
				float4(worldMatrix[1].xyz, 0.0f),
				float4(worldMatrix[2].xyz, 0.0f),
				float4(worldMatrix[3].xyz, 1.0f)
			);
		}
		
		inline float4x4 ExpandViewMatrix(in const float3x3 viewMatrix)
		{
			return float4x4(
				float4(viewMatrix[0], 0.0f),
				float4(viewMatrix[1], 0.0f),
				float4(viewMatrix[2], 0.0f),
				float4(0.0f, 0.0f, 0.0f, 1.0f)
			);
		}
			
		float3 GetTextureCubeReflectionVector(in const float2 uvCoords, in const BrawlerHLSL::TextureCubeFace faceID)
		{
			// We refer to the diagram given in the Direct3D 11.3 Specifications which describes the
			// coordinate system for each face of a TextureCube being accessed as a render target.
	
			// TextureCube reflection vectors exist in the [-1, 1]^3 cube, so we need to
			// appropriately scale the uvCoords from [0, 1] to [-1, 1].
			const float2 scaledUVCoords = (2.0f * uvCoords) - 1.0f;
	
			// We can avoid divergence by calculating the reflection vector using homogeneous
			// coordinates [u, v, w]. We store the transformation matrices in a static array
			// which is indexed based on textureCubeFaceIndex, and transform our scaledUVCoords
			// by the transformation matrix. To understand why these matrices were chosen, refer
			// to the Resource Type Illustrations diagram given at the beginning of Section 5 of the
			// Direct3D 11.3 Specifications.
	
			static const float3x3 UV_TRANSFORM_MATRIX_ARR[6] = {
				// TextureCubeFace::POSITIVE_X (+X Face, Index 0)
				float3x3(
					0.0f, 0.0f, -1.0f,
					0.0f, -1.0f, 0.0f,
					1.0f, 0.0f, 0.0f
				),
		
				// TextureCubeFace::NEGATIVE_X (-X Face, Index 1)
				float3x3(
					0.0f, 0.0f, 1.0f,
					0.0f, -1.0f, 0.0f,
					-1.0f, 0.0f, 0.0f
				),
		
				// TextureCubeFace::POSITIVE_Y (+Y Face, Index 2)
				float3x3(
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f
				),
		
				// TextureCubeFace::NEGATIVE_Y (-Y Face, Index 3)
				float3x3(
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, -1.0f,
					0.0f, -1.0f, 0.0f
				),
		
				// TextureCubeFace::POSITIVE_Z (+Z Face, Index 4)
				float3x3(
					1.0f, 0.0f, 0.0f,
					0.0f, -1.0f, 0.0f,
					0.0f, 0.0f, 1.0f
				),
		
				// TextureCubeFace::NEGATIVE_Z (-Z Face, Index 5)
				float3x3(
					-1.0f, 0.0f, 0.0f,
					0.0f, -1.0f, 0.0f,
					0.0f, 0.0f, -1.0f
				)
			};

			const uint transformMatrixArrIndex = (uint) (faceID);
			return mul(float3(scaledUVCoords, 1.0f), UV_TRANSFORM_MATRIX_ARR[transformMatrixArrIndex]);
		}
	}
}