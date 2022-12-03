#include "MeshTypes.hlsli"
#include "ViewTypes.hlsli"
#include "GPUSceneLimits.hlsli"
#include "LightDescriptor.hlsli"
#include "PointLight.hlsli"
#include "SpotLight.hlsli"
#include "MaterialDescriptor.hlsli"

// ================================================================================
// Bindless Types
//
// NOTE: By tradition, spaces greater than space0 are reserved for bindless SRV
// declarations.
// ================================================================================

namespace IMPL
{
	StructuredBuffer<BrawlerHLSL::PackedStaticVertex> Bindless_GlobalVertexBuffer[] : register(t0, space1);
	Buffer<uint> Bindless_GlobalUIntBuffer[] : register(t0, space2);
	StructuredBuffer<BrawlerHLSL::ModelInstanceTransformData> Bindless_GlobalModelInstanceTransformDataBuffer[] : register(t0, space3);
	StructuredBuffer<BrawlerHLSL::ViewTransformData> Bindless_GlobalViewTransformDataBuffer[] : register(t0, space4);
	StructuredBuffer<BrawlerHLSL::ViewDimensionsData> Bindless_GlobalViewDimensionsDataBuffer[] : register(t0, space5);
	StructuredBuffer<BrawlerHLSL::PointLight> Bindless_GlobalPointLightBuffer[] : register(t0, space6);
	StructuredBuffer<BrawlerHLSL::SpotLight> Bindless_GlobalSpotLightBuffer[] : register(t0, space7);
	StructuredBuffer<BrawlerHLSL::MeshDescriptor> Bindless_GlobalMeshDescriptorBuffer[] : register(t0, space8);
	StructuredBuffer<BrawlerHLSL::MaterialDescriptor> Bindless_GlobalMaterialDescriptorBuffer[] : register(t0, space9);
	
	Texture2D<float> Bindless_GlobalTexture2DFloatArray[] : register(t0, space10);
	Texture2D<uint> Bindless_GlobalTexture2DUInt[] : register(t0, space11);
}
	
namespace IMPL
{
	// NOTE: The values given here should match the underlying values of the GPUSceneBufferID
	// enumeration values found in GPUSceneBufferID.ixx.
		
	static const uint BINDLESS_GLOBAL_VERTEX_BUFFER_INDEX = 0;
	static const uint BINDLESS_MODEL_INSTANCE_TRANSFORM_DATA_BUFFER_INDEX = 1;
	static const uint BINDLESS_VIEW_TRANSFORM_DATA_BUFFER_INDEX = 2;
	static const uint BINDLESS_VIEW_DIMENSIONS_DATA_BUFFER_INDEX = 3;
	static const uint BINDLESS_LIGHT_DESCRIPTOR_BUFFER_INDEX = 4;
	static const uint BINDLESS_POINT_LIGHT_BUFFER_INDEX = 5;
	static const uint BINDLESS_SPOTLIGHT_BUFFER_INDEX = 6;
	static const uint BINDLESS_MODEL_INSTANCE_DESCRIPTOR_BUFFER_INDEX = 7;
	static const uint BINDLESS_MATERIAL_DESCRIPTOR_BUFFER_INDEX = 8;
	static const uint BINDLESS_VIEW_DESCRIPTOR_BUFFER_INDEX = 9;
}

namespace BrawlerHLSL
{
	namespace Bindless
	{
		BrawlerHLSL::PackedStaticVertex GetGlobalVertexBufferVertex(in const uint vertexID)
		{
			StructuredBuffer<BrawlerHLSL::PackedStaticVertex> packedVertexBuffer = IMPL::Bindless_GlobalVertexBuffer[IMPL::BINDLESS_GLOBAL_VERTEX_BUFFER_INDEX];
				
			return packedVertexBuffer[NonUniformResourceIndex(vertexID)];
		}
			
		uint GetGlobalIndexBufferIndex(in const uint indexID)
		{
			Buffer<uint> indexBuffer = IMPL::Bindless_GlobalUIntBuffer[IMPL::BINDLESS_GLOBAL_INDEX_BUFFER_INDEX];
				
			return indexBuffer[NonUniformResourceIndex(indexID)];
		}
			
		BrawlerHLSL::ModelInstanceTransformData GetGlobalModelInstanceTransformData(in const uint modelInstanceID)
		{
			StructuredBuffer<BrawlerHLSL::ModelInstanceTransformData > transformDataBuffer = IMPL::Bindless_GlobalModelInstanceTransformDataBuffer[IMPL::BINDLESS_MODEL_INSTANCE_TRANSFORM_DATA_BUFFER_INDEX];

			return transformDataBuffer[NonUniformResourceIndex(modelInstanceID)];
		}

		BrawlerHLSL::ViewTransformData GetGlobalViewTransformData(in const uint viewID)
		{
			StructuredBuffer<BrawlerHLSL::ViewTransformData> viewTransformDataBuffer = IMPL::Bindless_GlobalViewTransformDataBuffer[IMPL::BINDLESS_VIEW_TRANSFORM_DATA_BUFFER_INDEX];

			return viewTransformDataBuffer[NonUniformResourceIndex(viewID)];
		}

		BrawlerHLSL::ViewDimensionsData GetGlobalViewDimensionsData(in const uint viewID)
		{
			StructuredBuffer<BrawlerHLSL::ViewDimensionsData> viewDimensionsDataBuffer = IMPL::Bindless_GlobalViewDimensionsDataBuffer[IMPL::BINDLESS_VIEW_DIMENSIONS_DATA_BUFFER_INDEX];

			return viewDimensionsDataBuffer[NonUniformResourceIndex(viewID)];
		}
		
		BrawlerHLSL::LightDescriptor GetGlobalLightDescriptor(in const uint lightDescriptorID)
		{
			Buffer<uint> globalPackedLightDescriptorBuffer = IMPL::Bindless_GlobalUIntBuffer[IMPL::BINDLESS_LIGHT_DESCRIPTOR_BUFFER_INDEX];

			const uint packedLightDescriptorValue = globalPackedLightDescriptorBuffer[NonUniformResourceIndex(lightDescriptorID)];
			
			BrawlerHLSL::LightDescriptor lightDescriptor;
			lightDescriptor.LightBufferIndex = (packedLightDescriptorValue >> 8);
			lightDescriptor.TypeID = BrawlerHLSL::LightType((packedLightDescriptorValue >> 1) & 0x7F);
			lightDescriptor.IsValid = ((packedLightDescriptorValue & 0x1) != 0);
			
			return lightDescriptor;
		}
		
		BrawlerHLSL::PointLight GetGlobalPointLight(in const uint pointLightID)
		{
			StructuredBuffer<BrawlerHLSL::PointLight> globalPointLightBuffer = IMPL::Bindless_GlobalPointLightBuffer[IMPL::BINDLESS_POINT_LIGHT_BUFFER_INDEX];

			return globalPointLightBuffer[NonUniformResourceIndex(pointLightID)];
		}
		
		BrawlerHLSL::SpotLight GetGlobalSpotLight(in const uint spotLightID)
		{
			StructuredBuffer<BrawlerHLSL::SpotLight> globalSpotLightBuffer = IMPL::Bindless_GlobalSpotLightBuffer[IMPL::BINDLESS_SPOTLIGHT_BUFFER_INDEX];
			
			return globalSpotLightBuffer[NonUniformResourceIndex(spotLightID)];
		}
		
		BrawlerHLSL::ModelInstanceDescriptor GetGlobalModelInstanceDescriptor(in const uint modelInstanceDescriptorID)
		{
			Buffer<uint> globalPackedModelInstanceDescriptorBuffer = IMPL::Bindless_GlobalUIntBuffer[IMPL::BINDLESS_MODEL_INSTANCE_DESCRIPTOR_BUFFER_INDEX];
			
			const uint packedModelInstanceDescriptorValue = globalPackedModelInstanceDescriptorBuffer[NonUniformResourceIndex(modelInstanceDescriptorID)];
			
			BrawlerHLSL::ModelInstanceDescriptor modelInstanceDescriptor;
			modelInstanceDescriptor.TransformDataBufferIndex = (packedModelInstanceDescriptorValue >> 16);
			modelInstanceDescriptor.MeshDescriptorBufferID = ((packedModelInstanceDescriptorValue >> 1) & 0x7FFF);
			modelInstanceDescriptor.IsValid = ((packedModelInstanceDescriptorValue & 0x1) != 0);
			
			return modelInstanceDescriptor;
		}
		
		StructuredBuffer<BrawlerHLSL::MeshDescriptor> GetGlobalMeshDescriptorBuffer(in const uint meshDescriptorBufferID)
		{
			return IMPL::Bindless_GlobalMeshDescriptorBuffer[NonUniformResourceIndex(meshDescriptorBufferID)];
		}
		
		BrawlerHLSL::MaterialDescriptor GetGlobalMaterialDescriptor(in const uint materialDescriptorID)
		{
			StructuredBuffer<BrawlerHLSL::MaterialDescriptor> globalMaterialDescriptorBuffer = IMPL::Bindless_GlobalMaterialDescriptorBuffer[IMPL::BINDLESS_MATERIAL_DESCRIPTOR_BUFFER_INDEX];
			
			return globalMaterialDescriptorBuffer[NonUniformResourceIndex(materialDescriptorID)];
		}
		
		BrawlerHLSL::ViewDescriptor GetGlobalViewDescriptor(in const uint viewID)
		{
			Buffer<uint> globalPackedViewDescriptorBuffer = IMPL::Bindless_GlobalUIntBuffer[IMPL::BINDLESS_VIEW_DESCRIPTOR_BUFFER_INDEX];
			
			const uint packedViewDescriptorValue = globalPackedViewDescriptorBuffer[NonUniformResourceIndex(viewID)];
			
			BrawlerHLSL::ViewDescriptor viewDescriptor;
			viewDescriptor.ViewTransformBufferIndex = (packedViewDescriptorValue >> 20);
			viewDescriptor.ViewDimensionsBufferIndex = ((packedViewDescriptorValue >> 8) & 0xFFF);
			
			return viewDescriptor;
		}
	}
}

namespace IMPL
{
	template <typename T>
	struct TextureFinder
	{};
	
	template <>
	struct TextureFinder<Texture2D<float>>
	{
		static Texture2D<float> GetBindlessTexture(in const uint textureID)
		{
			return Bindless_GlobalTexture2DFloatArray[NonUniformResourceIndex(textureID)];
		}
	};
	
	template <>
	struct TextureFinder<Texture2D<uint>>
	{
		static Texture2D<uint> GetBindlessTexture(in const uint textureID)
		{
			return Bindless_GlobalTexture2DUIntArray[NonUniformResourceIndex(textureID)];
		}
	};
}

namespace BrawlerHLSL
{
	namespace Bindless
	{
		template <typename T>
		T GetBindlessTexture(in const uint textureID)
		{
			return IMPL::TextureFinder<T>::GetBindlessTexture(textureID);
		}
	}
}