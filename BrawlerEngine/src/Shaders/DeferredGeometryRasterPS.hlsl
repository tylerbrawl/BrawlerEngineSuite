#include "DeferredGeometryRasterTypes.hlsli"
#include "BindlessDescriptors.hlsli"
#include "MaterialDescriptor.hlsli"
#include "GPUSceneLimits.hlsli"
#include "NormalPacking.hlsli"

struct MaterialData
{
	Texture2D<float4> BaseColorTexture;
	Texture2D<float4> NormalMap;
	Texture2D<float4> RoughnessMap;
	Texture2D<float4> MetallicMap;
};

struct SurfaceData
{
	float3 BaseColor;
	float2 EncodedNormalWS;
	float GGXRoughnessSquared;
	bool IsMetallic;
};

MaterialData GetMaterialData(in const BrawlerHLSL::MaterialDescriptor materialDescriptor)
{
	MaterialData materialData;
	materialData.BaseColorTexture = BrawlerHLSL::Bindless::GetBindlessTexture<Texture2D<float4>>(materialDescriptor.BaseColorTextureSRVIndex);
	materialData.NormalMap = BrawlerHLSL::Bindless::GetBindlessTexture<Texture2D<float4>>(materialDescriptor.NormalMapSRVIndex);
	materialData.RoughnessMap = BrawlerHLSL::Bindless::GetBindlessTexture<Texture2D<float4>>(materialDescriptor.RoughnessTextureSRVIndex);
	materialData.MetallicMap = BrawlerHLSL::Bindless::GetBindlessTexture<Texture2D<float4>>(materialDescriptor.MetallicTextureSRVIndex);
	
	return materialData;
}

SurfaceData GetSurfaceData(in const BrawlerHLSL::DeferredGeometryRasterPSInput input)
{
	const BrawlerHLSL::MaterialDescriptor materialDescriptor = BrawlerHLSL::Bindless::GetGlobalMaterialDescriptor(input.MaterialDescriptorBufferIndex);
	const MaterialData materialData = GetMaterialData(materialDescriptor);
	
	SurfaceData surfaceData;
	
	// According to the MSDN, out-of-bounds descriptor table indexing results
	// in undefined behavior. (This is different from out-of-bounds buffer reads/writes,
	// which may have defined behavior depending on whether the buffer is bound using
	// a descriptor table versus as a root descriptor.)
	//
	// (The source for this information is at 
	// https://learn.microsoft.com/en-us/windows/win32/direct3d12/advanced-use-of-descriptor-tables#out-of-bounds-indexing.)
	//
	// To avoid the undefined behavior, then, we need to use the [branch] attribute
	// to force the evaluation of one or the other side of the if-statement for each.
	// (Whether or not we will get a lot of divergence completely depends on how the
	// GPU schedules pixel shader invocations within waves.)
	//
	// This does imply that we'll be entering a potentially divergent control flow,
	// which prevents the implicit calculation of UV gradients. We'll need to get those
	// here.
	const float2 ddxUVCoords = ddx_fine(input.UVCoords);
	const float2 ddyUVCoords = ddy_fine(input.UVCoords);
	
	// TODO: Our current material setup is inefficient for numerous reasons:
	//
	//   1. Our textures are loaded as .png files, which are not in the proper format for
	//      GPU texture uploads.
	//
	//   2. Most .png files contain channels which are not needed for a specific texture
	//      (e.g., a normal map can get away with two channels, and a roughness map needs
	//      only one for isotropic BRDF evaluation).
	//
	//   3. Some of the data within our textures are not optimized for our system (e.g.,
	//      our roughness maps could directly store GGX roughness squared values, but they
	//      do not).
	
	[branch]
	if (materialDescriptor.HasBaseColorTexture())
	{
		Texture2D<float4> baseColorTexture = BrawlerHLSL::Bindless::GetBindlessTexture<Texture2D<float4>>(materialDescriptor.BaseColorTextureSRVIndex);
		surfaceData.BaseColor = baseColorTexture.SampleGrad(
			AnisotropicWrapSampler,
			input.UVCoords,
			ddxUVCoords,
			ddyUVCoords
		).xyz;
	}
	else
		surfaceData.BaseColor = float3(0.0f, 0.0f, 0.0f);
	
	float3 normalWS;
	
	[branch]
	if (materialDescriptor.HasNormalMap())
	{
		Texture2D<float4> normalMap = BrawlerHLSL::Bindless::GetBindlessTexture<Texture2D<float4>>(materialDescriptor.NormalMapSRVIndex);
		float2 sampledWorldNormal = normalMap.SampleGrad(
			BilinearWrapSampler,
			input.UVCoords,
			ddxUVCoords,
			ddyUVCoords
		).xy;

		// The normal map values are stored as R8G8B8A8_UNORM values, but the actual values are
		// in the range [-1, 1]. So, we do the conversion here.
		sampledWorldNormal = mad(sampledWorldNormal, 2.0f, -1.0f);
		
		// The normal map doesn't explicitly store a Z-value. Instead, we infer this value by
		// assuming that the values stored in the normal map are the X- and Y-values of a
		// unit normal vector.
		//
		// We do this using the following calculation:
		//
		// x^2 + y^2 + z^2 = 1 <-> z = +/- sqrt(1.0f - x^2 - y^2)
		//
		// This would leave us with two possible values for the Z-value. However, since normal
		// maps are meant to store normal values contained within the hemisphere above the xy-plane
		// in texture space, we can safely disregard the negative value, since that would be in the
		// hemisphere below this plane.
		const float sampledWorldNormalZ = sqrt(1.0f - dot(sampledWorldNormal));
		
		const float3 worldNormalTS = float3(sampledWorldNormal, sampledWorldNormalZ);
		normalWS = normalize(mul(worldNormalTS, float3x3(input.TangentWS, input.BitangentWS, input.NormalWS)));
	}
	else
		normalWS = input.NormalWS;
	
	surfaceData.EncodedNormalWS = PackNormal(normalWS);
	
	[branch]
	if (materialDescriptor.HasRoughnessMap())
	{
		// Just like pretty much every other real-time rendering application, we use the
		// roughness mapping proposed by Burley in "Physically Based Shading at Disney:"
		//
		// a_g = UserRoughness^2
		//
		// where a_g is the GGX roughness value (pretend that the a is an alpha character,
		// please).
		Texture2D<float4> roughnessMap = BrawlerHLSL::Bindless::GetBindlessTexture<Texture2D<float4>>(materialDescriptor.RoughnessTextureSRVIndex);
		const float userRoughness = roughnessMap.SampleGrad(
			BilinearWrapSampler,
			input.UVCoords,
			ddxUVCoords,
			ddyUVCoords
		).x;

		const float ggxRoughness = (userRoughness * userRoughness);
		surfaceData.GGXRoughnessSquared = (ggxRoughness * ggxRoughness);
	}
	else
		surfaceData.GGXRoughnessSquared = 0.0625f;  // UserRoughness = 0.5f, GGXRoughness = (0.5f)^2
	
	[branch]
	if (materialDescriptor.HasMetallicMap())
	{
		Texture2D<float4> metallicMap = BrawlerHLSL::Bindless::GetBindlessTexture<Texture2D<float4>>(materialDescriptor.MetallicTextureSRVIndex);
		const float metallicValue = metallicMap.SampleGrad(
			BilinearWrapSampler,
			input.UVCoords,
			ddxUVCoords,
			ddyUVCoords
		).x;

		surfaceData.IsMetallic = (metallicValue > 0.0f);
	}
	else
		surfaceData.IsMetallic = false;
	
	return surfaceData;
}

[earlydepthstencil]
BrawlerHLSL::DeferredGeometryRasterPSOutput main(in BrawlerHLSL::DeferredGeometryRasterPSInput input)
{
	// Re-normalize the tangent frame, since interpolation across the triangle can result
	// in denormalization.
	input.TangentWS = normalize(input.TangentWS);
	input.BitangentWS = normalize(input.BitangentWS);
	input.NormalWS = normalize(input.NormalWS);
	
	const SurfaceData surfaceData = GetSurfaceData(input);
	
	BrawlerHLSL::DeferredGeometryRasterPSOutput output;
	output.BaseColorRoughnessGBuffer = float4(surfaceData.BaseColor, surfaceData.GGXRoughnessSquared);
	output.EncodedNormalGBuffer = surfaceData.EncodedNormalWS;
	output.MetallicGBuffer = (surfaceData.IsMetallic ? 1 : 0);
	
	return output;
}