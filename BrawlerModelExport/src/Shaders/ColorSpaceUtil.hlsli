namespace Util
{
	namespace ColorSpace
	{
		// The exact sRGB equations can be found at https://entropymine.com/imageworsener/srgbformula/.
		// The approximations were taken from https://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html.

		float4 LinearToSRGB(in const float4 linearColor)
		{
#ifdef __EXACT_SRGB__
			return float4(select(linearColor.xyz <= 0.0031308f, (linearColor.xyz * 12.92), 1.055f * pow(linearColor.xyz, 1.0f / 2.4f) - 0.055f), linearColor.w);
#else
			return float4(max(1.055f * pow(linearColor.xyz, 0.416666667f) - 0.055, 0), linearColor.w);
#endif
		}

		float4 SRGBToLinear(in const float4 srgbColor)
		{
#ifdef __EXACT_SRGB__
			return float4(select(srgbColor.xyz <= 0.04045f, (srgbColor.xyz / 12.92f), pow((srgbColor.xyz + 0.055f) / 1.055f, 2.4)), srgbColor.w);
#else
			float3 linearColor = mad(srgbColor.xyz, 0.305306011f, 0.682171111f);
			linearColor = mad(srgbColor.xyz, linearColor, 0.012522878f);
			linearColor *= srgbColor.xyz;
	
			return float4(linearColor, srgbColor.w);
#endif
		}
	}
}