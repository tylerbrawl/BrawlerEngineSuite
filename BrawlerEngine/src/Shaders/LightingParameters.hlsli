namespace BrawlerHLSL
{
	struct SurfaceParameters
	{
		/// <summary>
		/// This is the position in world space at which shading is taking place.
		///
		/// You will not be able to get this value from a G-buffer. Instead, the
		/// world space position must be inferred by using the depth buffer and
		/// the inverse view-projection matrix (VP)^-1 (along with the obligatory
		/// perspective divide, of course).
		/// </summary>
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
		/// This is the subsurface albedo of the surface being shaded. It is used to
		/// evaluate diffuse reflectance during evaluation of the BRDF.
		/// </summary>
		float3 SubsurfaceAlbedo;
		
		/// <summary>
		/// This is the "specular color" of the surface being shaded. It is used to
		/// evaluate specular reflectance during evaluation of the BRDF.
		///
		/// The term "specular color" is a more intuitive name for this, but it doesn't
		/// really describe its purpose. From a theoretical perspective, F_0 describes
		/// the reflectance of incoming light which arrives at an angle of 0* to the
		/// normal of the surface. As this angle increases, the reflectance also
		/// increases in a manner described by the Fresnel equations. At an angle of 90*,
		/// light at all frequencies is reflected back (i.e., F_90 = [1 1 1]).
		/// </summary>
		float3 F_0;
		
		/// <summary>
		/// Using the terminology described in Real-Time Rendering Fourth Edition, this
		/// value is equivalent to (a_g)^2 (here, a represents the greek letter alpha).
		///
		/// The value should be squared because this makes certain calculations more
		/// efficient.
		/// </summary>
		float GGXRoughnessSquared;
	};
	
	struct LightingParameters : SurfaceParameters
	{
		/// <summary>
		/// This is actually the absolute value of the dot product between
		/// N and V, with a small epsilon value (0.00001) to prevent divide-
		/// by-zero errors.
		///
		/// This trick is used by the Frostbite Engine. Please use this value
		/// instead of calculating dot(N, V) manually.
		/// </summary>
		float NDotV;
		
		/// <summary>
		/// This is the (normalized) light vector in world space. Let Light be the PointLight
		/// instance which represents the light whose incoming luminance is to be solved
		/// for. Then, L is calculated as follows:
		///
		/// L = ((Light.PositionWS - SurfacePosWS) / ||(Light.PositionWS - SurfacePosWS)||)
		/// </summary>
		float3 L;
		
		/// <summary>
		/// This is actually the value of dot(N, L) clamped to zero. Please use this value
		/// instead of calculating dot(N, L) manually.
		/// </summary>
		float NDotL;
		
		/// <summary>
		/// This is the distance from the light to the point on the surface being shaded *SQUARED*.
		/// The square root of this value is used to normalize L, but it is also needed for 
		/// inverse-square light attenuation.
		/// </summary>
		float LightDistanceSquared;
		
		/// <summary>
		/// This is the (normalized) half vector in world space. It is calculated as follows:
		///
		/// H = (L + V) / ||(L + V)||
		/// </summary>
		float3 H;
		
		/// <summary>
		/// This is actually the value of dot(L, H) clamped to zero. Please use this value
		/// instead of calculating dot(L, H) manually.
		/// </summary>
		float LDotH;
	};
}