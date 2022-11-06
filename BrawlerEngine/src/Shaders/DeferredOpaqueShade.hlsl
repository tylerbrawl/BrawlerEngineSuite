#include "LightingParameters.hlsli"
#include "BRDF.hlsli"
#include "BindlessDescriptors.hlsli"
#include "NormalPacking.hlsli"
#include "GPUSceneLimits.hlsli"
#include "LightDescriptor.hlsli"
#include "PointLight.hlsli"
#include "SpotLight.hlsli"
#include "MathConstants.hlsli"

Texture2D<float4> BaseColorRoughnessGBuffer : register(t0, space0);
Texture2D<float2> EncodedNormalGBuffer : register(t1, space0);
Texture2D<uint> MetallicGBuffer : register(t2, space0);

Texture2D<float> DepthBuffer : register(t3, space0);

struct ShadingConstantsInfo
{
	float2 InverseOutputTextureDimensions;
	uint ViewID;
};

ConstantBuffer<ShadingConstantsInfo> ShadingConstants : register(b0, space0);

RWTexture2D<float4> OutputTexture : register(u0, space0);

static const uint NUM_THREADS_X = 8;
static const uint NUM_THREADS_Y = 8;

float3 GetWorldSpacePosition(in const uint2 dispatchThreadID, in const BrawlerHLSL::ViewTransformData viewTransform)
{
	// Get the current lane's coordinates in NDC space.
	float4 currLanePosNDC = float4((float2(dispatchThreadID) + 0.5f) * ShadingConstants.InverseOutputTextureDimensions, DepthBuffer[NonUniformResourceIndex(dispatchThreadID)], 1.0f);
	currLanePosNDC.xy = mad(currLanePosNDC.xy, 2.0f, -1.0f);
	currLanePosNDC.y *= -1.0f;
	
	// Transform this position back into world space using the inverse view-projection
	// matrix of the view.
	float4 currLanePosWS = mul(currLanePosNDC, viewTransform.CurrentFrameInverseViewProjectionMatrix);
	
	// Perform the perspective divide.
	currLanePosWS.xyz /= currLanePosWS.w;
	
	return currLanePosWS.xyz;
}

BrawlerHLSL::SurfaceParameters GetSurfaceParameters(in const uint2 dispatchThreadID)
{
	BrawlerHLSL::SurfaceParameters surfaceParams;
	const BrawlerHLSL::ViewTransformData viewTransform = WaveReadLaneFirst(BrawlerHLSL::GetGlobalViewTransformData(ShadingConstants.ViewID));
	
	surfaceParams.SurfacePosWS = GetWorldSpacePosition(dispatchThreadID, viewTransform);
	surfaceParams.V = normalize(viewTransform.CurrentFrameViewOrigin - surfaceParams.SurfacePosWS);
	
	const float4 baseColorRoughnessValue = BaseColorRoughnessGBuffer[NonUniformResourceIndex(dispatchThreadID)];
	surfaceParams.GGXRoughnessSquared = baseColorRoughnessValue.a;
		
	const float metallicValue = saturate(MetallicGBuffer[NonUniformResourceIndex(dispatchThreadID)].r);
		
	// From Real-Time Rendering, 4th Edition: Section 9.5.2, Pg. 322:
	//
	// "For unknown dielectrics, 0.04 is a reasonable default value [for F_0], not too
	// far off from most common materials."
	static const float3 DIELECTRIC_F_0 = float3(0.04f, 0.04f, 0.04f);
		
	const float3 baseColor = baseColor.rgb;
	surfaceParams.SubsurfaceAlbedo = lerp(baseColor, 0.0f, metallicValue);
	surfaceParams.F_0 = lerp(DIELECTRIC_F_0, baseColor, metallicValue);
	
	const float2 encodedNormalValue = EncodedNormalGBuffer[NonUniformResourceIndex(dispatchThreadID)].rg;
	surfaceParams.N = UnpackNormal(encodedNormalValue);
	
	return surfaceParams;
}

float3 CalculatePointLightLuminance(in const BrawlerHLSL::SurfaceParameters surfaceParams, in const uint pointLightID)
{
	const BrawlerHLSL::PointLight currPointLight = BrawlerHLSL::GetGlobalPointLight(pointLightID);
	const BrawlerHLSL::LightingParameters lightingParams = currPointLight.CreateLightingParameters(surfaceParams);
	
	const float3 brdfReflectance = BrawlerHLSL::BRDF::EvaluateBRDF(lightingParams);
	const float3 L_i = currPointLight.CalculateAttenuatedLuminance(lightingParams);
	
	// TODO: Modify the returned value based on visibility using shadow maps.
	return (brdfReflectance * L_i * lightingParams.NDotL);
}

float3 CalculateSpotLightLuminance(in const BrawlerHLSL::SurfaceParameters surfaceParams, in const uint spotLightID)
{
	const BrawlerHLSL::SpotLight currSpotLight = BrawlerHLSL::GetGlobalSpotLight(spotLightID);
	const BrawlerHLSL::LightingParameters lightingParams = currSpotLight.CreateLightingParameters(surfaceParams);
	
	const float3 brdfReflectance = BrawlerHLSL::BRDF::EvaluateBRDF(lightingParams);
	const float3 L_i = currSpotLight.CalculateAttenuatedLuminance(lightingParams);
	
	// TODO: Modify the returned value based on visibility using shadow maps.
	return (brdfReflectance * L_i * lightingParams.NDotL);
}

float3 CalculateLighting(in const BrawlerHLSL::SurfaceParameters surfaceParams)
{
	float3 currLuminance = 0.0f;
	
	// Yeah... we don't want to unroll this.
	// TODO: Add some form of light culling.
	[loop]
	for (uint i = 0; i < BrawlerHLSL::GPUSceneLimits::MAX_LIGHTS; ++i)
	{
		const BrawlerHLSL::LightDescriptor currLightDescriptor = WaveReadLaneFirst(BrawlerHLSL::GetGlobalLightDescriptor(i));
		
		[branch]  // Coherent
		if (!currLightDescriptor.IsValid)
			continue;
		
		BrawlerHLSL::LightingParameters lightingParams;
		
		[branch]  // Coherent
		switch (currLightDescriptor.TypeID)
		{
			case BrawlerHLSL::LightType::POINT_LIGHT:
			{
				currLuminance += CalculatePointLightLuminance(surfaceParams, currLightDescriptor.LightBufferIndex);
				break;
			}
			
			case BrawlerHLSL::LightType::SPOTLIGHT:
			{
				currLuminance += CalculateSpotLightLuminance(surfaceParams, currLightDescriptor.LightBufferIndex);
				break;
			}
			
			// TODO: Add support for spotlights and other light types.
		}
	}

	return (currLuminance * BrawlerHLSL::PI);
}

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, 1)]
void main(in const uint2 dispatchThreadID : SV_DispatchThreadID)
{
	const BrawlerHLSL::SurfaceParameters currLaneSurfaceParams = GetSurfaceParameters(dispatchThreadID);
	OutputTexture[NonUniformResourceIndex(dispatchThreadID)] = float4(CalculateLighting(currLaneSurfaceParams), 1.0f);
}