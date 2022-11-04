#include "MathUtil.hlsli"
#include "Fresnel.hlsli"
#include "LightingParameters.hlsli"
#include "MathConstants.hlsli"

/*
Here are some definitions relevant to microfacet theory:

- Shadowing: occlusion of a light source by microscale surface detail; refer to
  the image provided in [1] for a more intuitive description [1]

- Masking: occlusion of microscale surface facets from view by other facets;
  refer to the image provided in [1] for a more intuitive description [1]

- Normal Distribution Function (NDF): statistical distribution of microfacet
  surface normals m over the microgeometry surface area; typically represented 
  mathematically as D(m), where m is the microfacet normal. Intuitively, the
  NDF is like a histogram of microfacet normals. It has high values in directions
  where the microfacet normals are more likely to be pointing. [2]
  
- Masking Function: mathematical function which returns a scalar describing the
  fraction of microfacets with normal m that are visible along the view vector v;
  typically represented mathematically as G_1(m, v). The product G_1(m, v)D(m)
  is the distribution of visible normals. [3]

- Joint Masking-Shadowing Function: mathematical function derived from the masking
  function G_1(m, v) which gives the fraction of microfacets with normal m that
  are visible from two directions: the view vector v and the light vector l;
  typically represented mathematically as G_2(l, v, m). Using this in place of
  G_1(m, v) enables the BRDF to account for masking as well as shadowing, but
  not for interreflection between microfacets. [4]

----------------------------------------------------------------------------------------------------------------

References:

[1]: Real-Time Rendering 4th Edition: Section 9.6, Pg. 328, 329
[2]: Real-Time Rendering 4th Edition: Section 9.7, Pg. 332, 333
[3]: Real-Time Rendering 4th Edition: Section 9.7, Pg. 333
[4]: Real-Time Rendering 4th Edition: Section 9.7, Pg. 334, 335
*/

namespace IMPL
{
	namespace BRDF
	{
		enum class NDFType : uint
		{
			GGX_DISTRIBUTION
		};
		
		template <uint TypeNum>
		struct NDFInfo
		{};
		
		/*
		Notes on Functions Defined by NDFInfo Template Specializations:
		
		- NormalDistributionFunction() is the definition of D(m) for a given NDF. Although
		  one of the function's parameters is named "M" to represent a microfacet normal, it is only
		  ever given the value of h, the half vector. This is because microfacet BRDFs assume
		  that all microfacets can be represented as an arbitrarily small Fresnel mirror;
		  in that case, only when the microfacet normal m is aligned with the half vector h
		  will any light be reflected into the direction specified by the view vector v. (This
		  assumption explains why microfacet BRDFs cannot handle interreflections and thus
		  tend to appear darker than they should.) For more details, refer to Real-Time Rendering 
		  4th Edition: Section 9.8, Pg. 336, 337.
		
		- HeightCorrelatedSmithMaskingShadowing() is the definition for the following:
		
		  HeightCorrelatedSmithMaskingShadowing() = G_2(l, v, m) / (4 * abs(dot(n, l)) * abs(dot(n, v)))
		
		  Despite its name, it is *NOT* only returning the value of G_2(l, v, m). This is done to
		  allow for some optimizations, should this equation be simplified.
		*/
		
		template <>
		struct NDFInfo<uint(NDFType::GGX_DISTRIBUTION)>
		{
			static float NormalDistributionFunction(in const BrawlerHLSL::LightingParameters lightingParams, in const float3 M)
			{
				// Returns: D(m)
				
				const float NDotM = saturate(dot(lightingParams.N, M));
				const float numerator = (Util::Math::HeavisideStepFunction(NDotM) * lightingParams.GGXRoughnessSquared);
				
				float denominator = (1.0f + (NDotM * NDotM) * (lightingParams.GGXRoughnessSquared - 1));
				denominator *= denominator;
				denominator *= BrawlerHLSL::PI;
				
				return (numerator / denominator);
			}
			
			static float HeightCorrelatedSmithMaskingShadowing(in const BrawlerHLSL::LightingParameters lightingParams, in const float3 M)
			{
				// Returns: G_2(l, v, m) / (4 * abs(dot(n, l)) * abs(dot(n, v)))
				//
				// This is a simplified version of the above expression for the GGX Distribution,
				// provided by "Moving Frostbite to Physically Based Rendering 3.0."
				
				const float a = lightingParams.NDotL * (lightingParams.NDotL - lightingParams.GGXRoughnessSquared * lightingParams.NDotL);
				const float denominator_a = lightingParams.NDotV * sqrt(lightingParams.GGXRoughnessSquared + a);
				
				const float b = lightingParams.NDotV * (lightingParams.NDotV - lightingParams.GGXRoughnessSquared * lightingParams.NDotV);
				const float denominator_b = lightingParams.NDotL * sqrt(lightingParams.GGXRoughnessSquared + b);
				
				return (0.5f / (denominator_a + denominator_b));
			}
		};
	}
}

namespace IMPL
{
	namespace BRDF
	{
		// Change this to easily modify the NDF used for evaluating the specular BRDF.
		// This allows for easy (?) experimentation. (Well, you still have to re-compile
		// the shaders, then wait for a few minutes for a lot of the C++ program to compile
		// again thanks to the ridiculous "build system" being used.)
		static const NDFType CURRENT_SPECULAR_BRDF_NDF = NDFType::GGX_DISTRIBUTION;
	}
}

namespace BrawlerHLSL
{
	namespace BRDF
	{
		float3 CalculateSpecularBRDFTerm(in const BrawlerHLSL::LightingParameters lightingParams)
		{
			// From Real-Time Rendering 4th Edition: Section 9.8, Pg. 337:
			//
			// f_spec(l, v) = a / b
			// a = F(h, l) * G_2(l, v, h) * D(h)
			// b = 4 * abs(dot(n, l)) * abs(dot(n, v))
			// h = (l + v) / ||(l + v)||
			
			const float3 fresnelReflectance = CalculateSchlickFresnel(lightingParams.F_0, lightingParams.LDotH);
			const float ndfFactor = NDFInfo<uint(IMPL::BRDF::CURRENT_SPECULAR_BRDF_NDF)>::NormalDistributionFunction(lightingParams, lightingParams.H);
			const float maskingShadowingFactor = NDFInfo<uint(IMPL::BRDF::CURRENT_SPECULAR_BRDF_NDF)>::HeightCorrelatedSmithMaskingShadowing(lightingParams, lightingParams.H);
			
			return (fresnelReflectance * ndfFactor * maskingShadowingFactor);
		}
	}
}