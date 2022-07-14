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
	}
}