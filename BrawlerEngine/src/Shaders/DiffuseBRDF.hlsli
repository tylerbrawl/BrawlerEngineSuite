#pragma once
#include "LightingParameters.hlsli"
#include "MathConstants.hlsli"
#include "Fresnel.hlsli"

namespace IMPL
{
	namespace BRDF
	{
		enum class DiffuseBRDFModel : uint
		{
			LAMBERTIAN_DIFFUSE,
			DISNEY_BURLEY,
			DISNEY_RENORMALIZED
		};

		template <uint DiffuseModelNum>
		struct DiffuseBRDFModelInfo
		{};
		
		template <>
		struct DiffuseBRDFModelInfo<(uint) (DiffuseBRDFModel::LAMBERTIAN_DIFFUSE)>
		{
			static float3 CalculateDiffuseBRDFTerm(in const BrawlerHLSL::LightingParameters lightingParams)
			{
				return (lightingParams.SubsurfaceAlbedo * BrawlerHLSL::INVERSE_PI);
			}
		};

		template <>
		struct DiffuseBRDFModelInfo<(uint) (DiffuseBRDFModel::DISNEY_BURLEY)>
		{
			static float3 CalculateDiffuseBRDFTerm(in const BrawlerHLSL::LightingParameters lightingParams)
			{
				// This is the Disney diffuse BRDF model without the global subsurface scattering
				// term and without the renormalization provided in "Moving Frostbite to Physically
				// Based Rendering 3.0."
				//
				// The equation here follows the base diffuse model described in "Physically-Based
				// Shading at Disney."
				
				// It isn't necessarily clear from "Moving Frostbite to Physically Based Rendering 3.0"
				// how linear roughness is calculated. They mention that they use the "squaring"
				// remapping from "Physically-Based Shading at Disney." Looking at that paper, it is
				// revealed in Section 5.4 that the "perceptually linear roughness" referenced in the
				// Frostbite paper refers to the roughness value in the following equation:
				//
				// a_g = roughness^2 (= (a_lin)^2)
				//
				// where a_g is the GGX roughness value. Our LightingParams structure stores
				// GGXRoughnessSquared, so we can get the linear roughness as follows:
				//
				// a_g = sqrt(GGXRoughnessSquared)
				// a_lin = sqrt(a_g) -> (GGXRoughnessSquared)^(1/4)
				const float linearRoughness = pow(lightingParams.GGXRoughnessSquared, 0.25f);
				
				static const float3 DIFFUSE_F0 = float3(1.0f, 1.0f, 1.0f);
				
				const float f_d90 = 0.5f + (2.0f * linearRoughness * (lightingParams.LDotH * lightingParams.LDotH));
				const float3 F_D90 = float3(f_d90, f_d90, f_d90);
				
				const float3 lightScattering = BrawlerHLSL::BRDF::CalculateSchlickFresnel(F_D90, DIFFUSE_F0, lightingParams.NDotL);
				const float3 viewScattering = BrawlerHLSL::BRDF::CalculateSchlickFresnel(F_D90, DIFFUSE_F0, lightingParams.NDotV);
				
				const float3 lambertianDiffuse = (lightingParams.SubsurfaceAlbedo * BrawlerHLSL::INVERSE_PI);
				
				return (lambertianDiffuse * lightScattering * viewScattering);
			}
		};

		template <>
		struct DiffuseBRDFModelInfo<(uint) (DiffuseBRDFModel::DISNEY_RENORMALIZED)>
		{
			static float3 CalculateDiffuseBRDFTerm(in const BrawlerHLSL::LightingParameters lightingParams)
			{
				// This is the renormalized variant of the Disney diffuse BRDF model described in
				// "Moving Frostbite to Physically Based Rendering 3.0."
				
				const float linearRoughness = pow(lightingParams.GGXRoughnessSquared, 0.25f);
				
				static const float3 DIFFUSE_F0 = float3(1.0f, 1.0f, 1.0f);
				
				const float energyBias = lerp(0.0f, 0.5f, linearRoughness);
				const float energyFactor = lerp(1.0f, 1.0f / 1.51f, linearRoughness);
				const float f_d90 = energyBias + (2.0f * linearRoughness * (lightingParams.LDotH * lightingParams.LDotH));
				const float3 F_D90 = float3(f_d90, f_d90, f_d90);
				
				const float3 lightScattering = BrawlerHLSL::BRDF::CalculateSchlickFresnel(F_D90, DIFFUSE_F0, lightingParams.NDotL);
				const float3 viewScattering = BrawlerHLSL::BRDF::CalculateSchlickFresnel(F_D90, DIFFUSE_F0, lightingParams.NDotV);
				
				const float3 lambertianDiffuse = (lightingParams.SubsurfaceAlbedo * BrawlerHLSL::INVERSE_PI);
				
				return (lambertianDiffuse * lightScattering * viewScattering * energyFactor);
			}
		};
	}
}

namespace IMPL
{
	namespace BRDF
	{
		// Change this to easily modify the diffuse BRDF model used. This allows for easy (?) 
		// experimentation. (Well, you still have to re-compile the shaders, then wait for a few 
		// minutes for a lot of the C++ program to compile again thanks to the ridiculous "build 
		// system" being used.)
		static const DiffuseBRDFModel CURRENT_DIFFUSE_BRDF_MODEL = DiffuseBRDFModel::DISNEY_RENORMALIZED;
	}
}

namespace BrawlerHLSL
{
	namespace BRDF
	{
		float3 CalculateDiffuseBRDFTerm(in const BrawlerHLSL::LightingParameters lightingParams)
		{
			return IMPL::BRDF::DiffuseBRDFModelInfo<(uint) (IMPL::BRDF::CURRENT_DIFFUSE_BRDF_MODEL)>::CalculateDiffuseBRDFTerm(lightingParams);
		}
	}
}