#include "LightingParameters.hlsli"
#include "PunctualLight.hlsli"

namespace BrawlerHLSL
{
	class SpotLight : PunctualLight
	{
		/// <summary>
		/// This is the position, in world space, at which the spotlight is located.
		/// in the scene.
		/// </summary>
		float3 PositionWS;
		
		/// <summary>
		/// This is the luminous intensity, in Candelas (cd), of the light.
		///
		/// NOTE: Frostbite exposes luminous power (in lumens, or lm) to artists and converts
		/// these values to luminous intensity for spotlights as follows:
		///
		/// LuminousIntensity = LuminousPower lumens / PI steradians = (LuminousPower / PI) lm/sr = (LuminousPower / PI) cd
		///
		/// This is intentionally *NOT* mathematically correct; see the comments in
		/// SpotLight::CalculateAttenuatedLuminance() for more details.
		/// </summary>
		float LuminousIntensity;
		
		/// <summary>
		/// This is the color of the light expressed as an RGB triple in (linear) sRGB color
		/// space. (Future work would be to compute this value from, e.g., color temperature,
		/// but such conversions should never have to happen on the GPU.)
		/// </summary>
		float3 LightColor;
		
		/// <summary>
		/// This is (1.0f / MaxDistance)^2, where MaxDistance is the maximum distance, in meters, 
		/// which the light reaches. If one were to imagine the spotlight as a cone, then
		/// MaxDistance would be the magnitude of the *UNNORMALIZED* DirectionWS vector.
		///
		/// *NOTE*: MaxDistance *MUST* be at least 1 cm, which is the "size" of a punctual
		/// light in the Brawler Engine.
		/// </summary>
		float InverseMaxDistanceSquared;
		
		/// <summary>
		///	This is the (normalized) direction vector of the spotlight in world space. The direction
		/// vector describes the orientation of the spotlight.
		/// </summary>
		float3 DirectionWS;
		
		/// <summary>
		/// This is the value of cos(UmbraAngle), where UmbraAngle describes the angle from the vector
		/// DirectionWS to the maximum extent of the cone bounding volume of the spotlight.
		///
		/// This value is sometimes less formally referred to as the "outer angle" of the spotlight.
		/// </summary>
		float CosineUmbraAngle;
		
		/// <summary>
		/// This is the value of cos(PenumbraAngle), where PenumbraAngle describes the angle from the
		/// vector DirectionWS to the maximum extent of an imaginary inner cone such that for all
		/// points within said inner cone, angular attenuation has no effect on the luminance.
		///
		/// Necessarily, PenumbraAngle <= UmbraAngle (and thus CosinePenumbraAngle >= CosineUmbraAngle).
		///
		/// This value is sometimes less formally referred to as the "inner angle" of the spotlight.
		/// </summary>
		float CosinePenumbraAngle;
		
		float3 __Pad0;
		
		BrawlerHLSL::LightingParameters CreateLightingParameters(in const BrawlerHLSL::SurfaceParameters surfaceParams)
		{
			return CreateLightingParametersIMPL(surfaceParams, PositionWS);
		}
		
		float3 CalculateAttenuatedLuminance(in const BrawlerHLSL::LightingParameters shadingParams)
		{
			// From "Moving Frostbite to Physically Based Rendering 3.0"
			// and Real-Time Rendering 4th Edition - Equation 5.17:
			//
			// L_out = LightColor * LuminousIntensity * CalculateDistanceAttenuation() * angularAttenuation
			// angularAttenuation = saturate((a - c) / (b - c))^2
			// a = dot(DirectionWS, -L)
			// b = CosinePenumbraAngle
			// c = CosineUmbraAngle
			
			const float cosineLightAngle = dot(DirectionWS, -shadingParams.L);
			
			float angularAttenuation = saturate((cosineLightAngle - CosineUmbraAngle) / max(CosinePenumbraAngle - CosineUmbraAngle, 0.00001f));
			angularAttenuation *= angularAttenuation;
			
			return (LightColor * LuminousIntensity * CalculateDistanceAttenuation(shadingParams, InverseMaxDistanceSquared) * angularAttenuation);
		}
	};
}