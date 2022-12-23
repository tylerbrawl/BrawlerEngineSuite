#pragma once
#include "LightingParameters.hlsli"
#include "PunctualLight.hlsli"

// HLSL doesn't have access modifiers/encapsulation (i.e., public/private/protected access),
// so this is the best we can do. (It's still WAY better than GLSL, though!)

namespace BrawlerHLSL
{
	struct PointLight : PunctualLight
	{
		float3 PositionWS;
		
		/// <summary>
		/// This is (1.0f / MaxDistance)^2, where MaxDistance is the maximum distance, in meters, 
		/// which the light reaches. If one were to imagine the point light as a sphere, then
		/// MaxDistance would be the sphere's radius.
		///
		/// *NOTE*: MaxDistance *MUST* be at least 1 cm, which is the "size" of a punctual
		/// light in the Brawler Engine.
		/// </summary>
		float InverseMaxDistanceSquared;
		
		/// <summary>
		/// This is the color of the light expressed as an RGB triple in (linear) sRGB color
		/// space. The value is scaled by the luminous intensity, in Candelas (cd), of the
		/// light.
		///
		/// NOTE: Frostbite exposes luminous power (in lumens, or lm) to artists and converts
		/// these values to luminous intensity for point lights as follows:
		///
		/// LuminousIntensity = LuminousPower lumens / (4 * PI steradians) = (LuminousPower / (4 * PI)) lm/sr = (LuminousPower / (4 * PI)) cd
		///
		/// (Future work would be to compute this value from, e.g., color temperature,
		/// but such conversions should never have to happen on the GPU.)
		/// </summary>
		float3 ScaledLightColor;
		
		uint __Pad0;
		
		BrawlerHLSL::LightingParameters CreateLightingParameters(in const BrawlerHLSL::SurfaceParameters surfaceParams)
		{
			return CreateLightingParametersIMPL(surfaceParams, PositionWS);
		}
		
		float3 CalculateAttenuatedLuminance(in const BrawlerHLSL::LightingParameters shadingParams)
		{
			// From "Moving Frostbite to Physically Based Rendering 3.0" - 
			// Section 4.4:
			//
			// For Point Lights:
			//
			// L_out = f(v, l) * E = f(v, l) * L_in * saturate(dot(n, l))
			// L_in = LightColor * LuminousIntensity * CalculateDistanceAttenuation() = ScaledLightColor * CalculateDistanceAttenuation()
			//
			// (NOTE: The L_in equation isn't explicitly stated in the document directly; it
			// is inferred from both Equation 18 and the code given in Listing 4.)
			
			return (ScaledLightColor * CalculateDistanceAttenuation(shadingParams, InverseMaxDistanceSquared));
		}
	};
}