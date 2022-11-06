#include "LightingParameters.hlsli"

namespace BrawlerHLSL
{
	class PunctualLight
	{
		// We don't have protected access modifiers in HLSL (and thus no encapsulation),
		// so this is the best that we can do.
		
		static BrawlerHLSL::LightingParameters CreateLightingParametersIMPL(in const BrawlerHLSL::SurfaceParameters surfaceParams, in const float3 lightPosWS)
		{
			BrawlerHLSL::LightingParameters lightingParams = surfaceParams;
			lightingParams.NDotV = (abs(dot(lightingParams.N, lightingParams.V)) + 0.00001f);
			
			lightingParams.L = (lightPosWS - lightingParams.SurfacePosWS);
			lightingParams.LightDistanceSquared = dot(lightingParams.L, lightingParams.L);
			lightingParams.L /= sqrt(lightingParams.LightDistanceSquared);
			
			lightingParams.NDotL = saturate(dot(lightingParams.N, lightingParams.L));
			lightingParams.H = normalize(lightingParams.L + lightingParams.V);
			lightingParams.LDotH = saturate(dot(lightingParams.L, lightingParams.H));
			
			return lightingParams;
		}
		
		static float3 CalculateDistanceAttenuation(in const BrawlerHLSL::LightingParameters shadingParams, in const float inverseMaxDistanceSquared)
		{
			// From "Moving Frostbite to Physically Based Rendering 3.0"
			// and Real-Time Rendering 4th Edition - Equations 5.14, 5.13
			
			// In real-time rendering, distance attentuation for punctual lights is typically
			// composed of two separate but related components:
			//
			//   - Inverse-Square Law Attenuation
			//   - Windowing Function
			//
			// Of these two, only the first is physically based. The inverse-square law states
			// that as the distance r from a surface to a punctual light increases, the density
			// of light rays (and thus the incoming luminous intensity) decreases proportionally
			// to (1.0f / r^2).
			//
			// This implies, however, that no matter how far a surface is from a light, it will
			// still be affected by it. This would be disastrous for performance; we only want
			// lights to affect surfaces if doing so has a significant visual impact. Thus, we
			// typically use a windowing function to "cap" the distance a light can be from a
			// surface for it to illuminate it.
			
			// A punctual light has a "size" of 1 cm.
			static const float PUNCTUAL_LIGHT_MINIMUM_SIZE = 0.01;
			static const float PUNCTUAL_LIGHT_MINIMUM_SIZE_SQUARED = (PUNCTUAL_LIGHT_MINIMUM_SIZE * PUNCTUAL_LIGHT_MINIMUM_SIZE);
			
			const float distancesFactor = (shadingParams.LightDistanceSquared * inverseMaxDistanceSquared);
			
			float windowingFunction = saturate(1.0f - (distancesFactor * distancesFactor));
			windowingFunction *= windowingFunction;
			
			float invSquareAttenuation = (1.0f / max(shadingParams.LightDistanceSquared, PUNCTUAL_LIGHT_MINIMUM_SIZE_SQUARED));
			invSquareAttenuation *= invSquareAttenuation;
			
			return (windowingFunction * invSquareAttenuation);
		}
	};
}