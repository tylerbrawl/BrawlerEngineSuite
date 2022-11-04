#include "LightingParameters.hlsli"
#include "PunctualLight.hlsli"

// HLSL doesn't have access modifiers/encapsulation (i.e., public/private/protected access),
// so this is the best we can do. (It's still WAY better than GLSL, though!)
namespace IMPL
{
	struct PointLightLightingParameters : BrawlerHLSL::LightingParameters
	{
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
	};
}

namespace BrawlerHLSL
{
	class PointLight
	{
		float3 PositionWS;
		
		/// <summary>
		/// This is the color of the light expressed as an RGB triple in (linear) sRGB color
		/// space.
		/// </summary>
		float3 LightColor;
		
		/// <summary>
		/// This is the luminous intensity, in Candelas (cd), of the light.
		/// </summary>
		float LuminousIntensity;
		
		/// <summary>
		/// This is (1.0f / MaxDistance)^2, where MaxDistance is the maximum distance, in meters, 
		/// which the light reaches. If one were to imagine the point light as a sphere, then
		/// MaxDistance would be the sphere's radius.
		///
		/// *NOTE*: MaxDistance *MUST* be at least than 1 cm, which is the "size" of a punctual
		/// light in the Brawler Engine.
		/// </summary>
		float InverseMaxDistanceSquared;
		
		float3 CalculateIncomingLuminance(in const LightingParameters lightingParams)
		{
			// From "Moving Frostbite to Physically Based Rendering 3.0"
			
		}
		
		float3 CalculateAttenuatedLuminance(in const IMPL::PointLightLightingParameters shadingParams)
		{
			// From "Moving Frostbite to Physically Based Rendering 3.0"
			// and Real-Time Rendering 4th Edition - Equations 5.14, 5.13
			
			// A punctual light has a "size" of 1 cm.
			static const float PUNCTUAL_LIGHT_MINIMUM_SIZE = 0.01;
			static const float PUNCTUAL_LIGHT_MINIMUM_SIZE_SQUARED = (PUNCTUAL_LIGHT_MINIMUM_SIZE * PUNCTUAL_LIGHT_MINIMUM_SIZE);
			
			const float distancesFactor = (shadingParams.LightDistanceSquared * InverseMaxDistanceSquared);
			
			float windowingFunction = saturate(1.0f - (distancesFactor * distancesFactor));
			windowingFunction *= windowingFunction;
			
			float invSquareAttenuation = (1.0f / max(shadingParams.LightDistanceSquared, PUNCTUAL_LIGHT_MINIMUM_SIZE_SQUARED));
			invSquareAttenuation *= invSquareAttenuation;
			
			// From "Moving Frostbite to Physically Based Rendering 3.0" - 
			// Section 4.4:
			//
			// For Point Lights:
			//
			// L_out = f(v, l) * E = f(v, l) * L_in * saturate(dot(n, l))
			// L_in = LightColor * LuminousIntensity * windowingFunction * invSquareAttenuation
			//
			// (NOTE: The L_in equation isn't explicitly stated in the document directly; it
			// is inferred from both Equation 18 and the code given in Listing 4.)
			
			return (LightColor * LuminousIntensity * windowingFunction * invSquareAttenuation);
		}
		
		IMPL::PointLightLightingParameters CreatePointLightLightingParameters(in const LightingParameters lightingParams)
		{
			// TODO: Does this even work in HLSL? (IMPL::PointLightLightingParameters extends
			// LightingParameters.)
			IMPL::PointLightLightingParameters fullParams = lightingParams;
			
			fullParams.L = (PositionWS - fullParams.SurfacePosWS);
			fullParams.LightDistanceSquared = dot(fullParams.L, fullParams.L);
			fullParams.L /= sqrt(fullParams.LightDistanceSquared);
			
			fullParams.NDotL = saturate(dot(fullParams.N, fullParams.L));
			
			return fullParams;
		}
	};
}