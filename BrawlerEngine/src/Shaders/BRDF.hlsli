#pragma once
#include "LightingParameters.hlsli"
#include "DiffuseBRDF.hlsli"
#include "SpecularBRDF.hlsli"

namespace BrawlerHLSL
{
	namespace BRDF
	{
		float3 EvaluateBRDF(in const BrawlerHLSL::LightingParameters lightingParams)
		{
			return (CalculateDiffuseBRDFTerm(lightingParams) + CalculateSpecularBRDFTerm(lightingParams));
		}
	}
}