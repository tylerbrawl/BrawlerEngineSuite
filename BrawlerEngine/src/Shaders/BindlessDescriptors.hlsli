#include "MeshTypes.hlsli"
#include "ViewTypes.hlsli"
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
}
	
namespace IMPL
{
	static const uint BINDLESS_GLOBAL_VERTEX_BUFFER_INDEX = 0;
	static const uint BINDLESS_GLOBAL_INDEX_BUFFER_INDEX = 1;
	static const uint BINDLESS_MODEL_INSTANCE_TRANSFORM_DATA_BUFFER_INDEX = 2;
	static const uint BINDLESS_LOD_MESH_DATA_BUFFER_INDEX = 3;
	static const uint BINDLESS_VIEW_TRANSFORM_DATA_BUFFER_INDEX = 4;
	static const uint BINDLESS_VIEW_DIMENSIONS_DATA_BUFFER_INDEX = 5;
	static const uint BINDLESS_GLOBAL_TRIANGLE_CLUSTER_BUFFER_INDEX = 6;
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
			StructuredBuffer<BrawlerHLSL::ModelInstanceTransformData> transformDataBuffer = IMPL::Bindless_GlobalModelInstanceTransformDataBuffer[IMPL::BINDLESS_MODEL_INSTANCE_TRANSFORM_DATA_BUFFER_INDEX];

			return transformDataBuffer[NonUniformResourceIndex(modelInstanceID)];
		}

		BrawlerHLSL::LODMeshData GetGlobalModelInstanceLODMeshData(in const uint modelInstanceID)
		{
			StructuredBuffer<BrawlerHLSL::LODMeshData> lodMeshDataBuffer = IMPL::Bindless_GlobalLODMeshDataBuffer[IMPL::BINDLESS_LOD_MESH_DATA_BUFFER_INDEX];
				
			return lodMeshDataBuffer[NonUniformResourceIndex(modelInstanceID)];
		}

		BrawlerHLSL::ViewTransformData GetGlobalViewTransformData(in const uint viewID)
		{
			StructuredBuffer<BrawlerHLSL::ViewTransformData> viewTransformDataBuffer = IMPL::Bindless_GlobalViewTransformDataBuffer[IMPL::BINDLESS_VIEW_TRANSFORM_DATA_BUFFER_INDEX];

			return viewTransformDataBuffer[viewID];
		}

		BrawlerHLSL::ViewDimensionsData GetGlobalViewDimensionsData(in const uint viewID)
		{
			StructuredBuffer<BrawlerHLSL::ViewDimensionsData> viewDimensionsDataBuffer = IMPL::Bindless_GlobalViewDimensionsDataBuffer[IMPL::BINDLESS_VIEW_DIMENSIONS_DATA_BUFFER_INDEX];

			return viewDimensionsDataBuffer[BINDLESS_VIEW_DIMENSIONS_DATA_BUFFER_INDEX];
		}

		BrawlerHLSL::PackedTriangleCluster GetGlobalPackedTriangleCluster(in const uint clusterID)
		{
			StructuredBuffer<BrawlerHLSL::PackedTriangleCluster> triangleClusterBuffer = IMPL::Bindless_GlobalTriangleClusterBuffer[IMPL::BINDLESS_GLOBAL_TRIANGLE_CLUSTER_BUFFER_INDEX];

			return triangleClusterBuffer[NonUniformResourceIndex(clusterID)];
		}
	}
}