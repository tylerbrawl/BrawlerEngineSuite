#include "MeshTypes.hlsli"
#include "ViewTypes.hlsli"
#include "VirtualTextureDescription.hlsli"
#include "GlobalTextureDescription.hlsli"
#include "TriangleCluster.hlsli"
#include "GPUSceneLimits.hlsli"

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
	StructuredBuffer<BrawlerHLSL::LODMeshData> Bindless_GlobalLODMeshDataBuffer[] : register(t0, space4);
	StructuredBuffer<BrawlerHLSL::ViewTransformData> Bindless_GlobalViewTransformDataBuffer[] : register(t0, space5);
	StructuredBuffer<BrawlerHLSL::ViewDimensionsData> Bindless_GlobalViewDimensionsDataBuffer[] : register(t0, space6);
	StructuredBuffer<BrawlerHLSL::PackedTriangleCluster> Bindless_GlobalTriangleClusterBuffer[] : register(t0, space7);
	StructuredBuffer<BrawlerHLSL::VirtualTextureDescription> Bindless_GlobalVirtualTextureDescriptionBuffer[] : register(t0, space8);
	StructuredBuffer<BrawlerHLSL::GlobalTextureDescription> Bindless_GlobalGlobalTextureDescriptionBuffer[] : register(t0, space9);
	
	Texture2D<float> Bindless_GlobalTexture2DFloatArray[] : register(t0, space10);
	Texture2D<uint> Bindless_GlobalTexture2DUInt[] : register(t0, space11);
}
	
namespace IMPL
{
	// NOTE: The values given here should match the underlying values of the GPUSceneBufferID
	// enumeration values found in GPUSceneBufferID.ixx.
		
	static const uint BINDLESS_GLOBAL_VERTEX_BUFFER_INDEX = 0;
	static const uint BINDLESS_GLOBAL_INDEX_BUFFER_INDEX = 1;
	static const uint BINDLESS_MODEL_INSTANCE_TRANSFORM_DATA_BUFFER_INDEX = 2;
	static const uint BINDLESS_LOD_MESH_DATA_BUFFER_INDEX = 3;
	static const uint BINDLESS_VIEW_TRANSFORM_DATA_BUFFER_INDEX = 4;
	static const uint BINDLESS_VIEW_DIMENSIONS_DATA_BUFFER_INDEX = 5;
	static const uint BINDLESS_GLOBAL_TRIANGLE_CLUSTER_BUFFER_INDEX = 6;
	static const uint BINDLESS_LOD_MESH_DATA_INDEX_BUFFER_INDEX = 7;
	static const uint BINDLESS_VIRTUAL_TEXTURE_DESCRIPTION_BUFFER_INDEX = 8;
	static const uint BINDLESS_GLOBAL_TEXTURE_DESCRIPTION_BUFFER_INDEX = 9;
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

		BrawlerHLSL::LODMeshData GetGlobalModelInstanceLODMeshData(in const uint modelInstanceID)
		{
			Buffer<uint> lodMeshDataIndexBuffer = IMPL::Bindless_GlobalUIntBuffer[IMPL::BINDLESS_LOD_MESH_DATA_INDEX_BUFFER_INDEX];
			const uint modelInstanceLODMeshDataIndex = lodMeshDataIndexBuffer[NonUniformResourceIndex(modelInstanceID)];
				
			StructuredBuffer<BrawlerHLSL::LODMeshData> lodMeshDataBuffer = IMPL::Bindless_GlobalLODMeshDataBuffer[IMPL::BINDLESS_LOD_MESH_DATA_BUFFER_INDEX];
			return lodMeshDataBuffer[NonUniformResourceIndex(modelInstanceLODMeshDataIndex)];
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

		BrawlerHLSL::PackedTriangleCluster GetGlobalPackedTriangleCluster(in const uint clusterID)
		{
			StructuredBuffer<BrawlerHLSL::PackedTriangleCluster> triangleClusterBuffer = IMPL::Bindless_GlobalTriangleClusterBuffer[IMPL::BINDLESS_GLOBAL_TRIANGLE_CLUSTER_BUFFER_INDEX];

			return triangleClusterBuffer[NonUniformResourceIndex(clusterID)];
		}
			
		BrawlerHLSL::VirtualTextureDescription GetGlobalVirtualTextureDescription(in const uint virtualTextureID)
		{
			StructuredBuffer<BrawlerHLSL::VirtualTextureDescription> vtDescriptionBuffer = IMPL::Bindless_GlobalVirtualTextureDescriptionBuffer[IMPL::BINDLESS_VIRTUAL_TEXTURE_DESCRIPTION_BUFFER_INDEX];
				
			return vtDescriptionBuffer[NonUniformResourceIndex(virtualTextureID)];
		}
			
		BrawlerHLSL::GlobalTextureDescription GetGlobalTextureDescription(in const uint globalTextureID)
		{
			StructuredBuffer<BrawlerHLSL::GlobalTextureDescription> globalTextureDescriptionBuffer = IMPL::Bindless_GlobalGlobalTextureDescription[IMPL::BINDLESS_GLOBAL_TEXTURE_DESCRIPTION_BUFFER_INDEX];
				
			return globalTextureDescriptionBuffer[NonUniformResourceIndex(globalTextureID)];
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