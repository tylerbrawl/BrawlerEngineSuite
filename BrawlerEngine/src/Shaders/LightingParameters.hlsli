namespace BrawlerHLSL
{
	struct LightingParameters
	{
		float3 SurfacePosWS;
		
		/// <summary>
		/// This is the (normalized) normal vector in world space of the
		/// surface at the point being shaded (i.e., SurfacePosWS).
		/// </summary>
		float3 N;
		
		/// <summary>
		/// This is the (normalized) view vector in world space. Let
		/// C be the world space position of the relevant camera/view. Then,
		/// V is calculated as follows:
		///
		/// V = ((C - SurfacePosWS) / ||(C - SurfacePosWS)||)
		/// </summary>
		float3 V;
		
		/// <summary>
		/// This is actually the absolute value of the dot product between
		/// N and V, with a small epsilon value (0.00001) to prevent divide-
		/// by-zero errors.
		///
		/// This trick is used by the Frostbite Engine. Please use this value
		/// instead of calculating dot(N, V) manually.
		/// </summary>
		float NDotV;
	};
	
	LightingParameters CreateLightingParameters(in const float3 surfacePosWS, in const float3 surfaceNormalWS, in const float3 viewPosWS)
	{
		LightingParameters lightingParams;
		lightingParams.SurfacePosWS = surfacePosWS;
		lightingParams.N = surfaceNormalWS;
		lightingParams.V = normalize(viewPosWS - surfacePosWS);
		lightingParams.NDotV = (abs(dot(lightingParams.N, lightingParams.V)) + 0.00001f);
		
		return lightingParams;
	}
}