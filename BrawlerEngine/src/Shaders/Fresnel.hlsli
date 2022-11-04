namespace BrawlerHLSL
{
	namespace BRDF
	{
		float3 CalculateSchlickFresnel(in const float3 F_90, in const float3 F_0, in const float NDotL)
		{
			const float3 offset = (F_90 - F_0) * pow(1.0f - NDotL, 5.0f);
			return (F_0 + offset);
		}
		
		float3 CalculateSchlickFresnel(in const float3 F_0, in const float NDotL)
		{
			static const float3 DEFAULT_F_90 = float3(1.0f, 1.0f, 1.0f);
			return CalculateSchlickFresnel(DEFAULT_F_90, F_0, NDotL);
		}
	}
}