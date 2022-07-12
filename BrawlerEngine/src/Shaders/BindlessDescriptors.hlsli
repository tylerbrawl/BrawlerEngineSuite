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
	Buffer<uint> Bindless_GlobalIndexBuffer[] : register(t0, space2);
	StructuredBuffer<BrawlerHLSL::ModelInstanceTransformData> Bindless_GlobalModelInstanceTransformDataBuffer[] : register(t0, space3);
	StructuredBuffer<BrawlerHLSL::ModelInstanceLODMeshData> Bindless_GlobalModelInstanceLODMeshDataBuffer[] : register(t0, space4);
	StructuredBuffer<BrawlerHLSL::ViewTransformData> Bindless_GlobalViewTransformDataBuffer[] : register(t0, space5);
	StructuredBuffer<BrawlerHLSL::ViewDimensionsData> Bindless_GlobalViewDimensionsDataBuffer[] : register(t0, space6);
	StructuredBuffer<BrawlerHLSL::PackedTriangleCluster> Bindless_GlobalTriangleClusterBuffer[] : register(t0, space7);
	StructuredBuffer<BrawlerHLSL::GPUSceneState> Bindless_GlobalGPUSceneStateBuffer[] : register(t0, space8);
}

namespace BrawlerHLSL
{
	namespace Bindless
	{
		BrawlerHLSL::PackedStaticVertex GetGlobalVertexBufferVertex(in const uint vertexID)
		{
			static const uint BINDLESS_GLOBAL_VERTEX_BUFFER_INDEX = 0;
			StructuredBuffer<BrawlerHLSL::PackedStaticVertex> packedVertexBuffer = IMPL::Bindless_GlobalVertexBuffer[BINDLESS_GLOBAL_VERTEX_BUFFER_INDEX];
				
			return packedVertexBuffer[NonUniformResourceIndex(vertexID)];
		}
			
		uint GetGlobalIndexBufferIndex(in const uint indexID)
		{
			static const uint BINDLESS_GLOBAL_INDEX_BUFFER_INDEX = 1;
			Buffer<uint> indexBuffer = IMPL::Bindless_GlobalUIntBuffer[BINDLESS_GLOBAL_INDEX_BUFFER_INDEX];
				
			return indexBuffer[NonUniformResourceIndex(indexID)];
		}
			
		BrawlerHLSL::ModelInstanceTransformData GetGlobalModelInstanceTransformData(in const uint modelInstanceID)
		{
			static const uint BINDLESS_MODEL_INSTANCE_TRANSFORM_DATA_BUFFER_INDEX = 2;
			StructuredBuffer<BrawlerHLSL::ModelInstanceTransformData> transformDataBuffer = IMPL::Bindless_GlobalModelInstanceTransformDataBuffer[BINDLESS_MODEL_INSTANCE_TRANSFORM_DATA_BUFFER_INDEX];

			return transformDataBuffer[NonUniformResourceIndex(modelInstanceID)];
		}

		BrawlerHLSL::ModelInstanceLODMeshData GetGlobalModelInstanceLODMeshData(in const uint modelInstanceID)
		{
			static const uint BINDLESS_MODEL_INSTANCE_LOD_MESH_DATA_BUFFER_INDEX = 3;
			StructuredBuffer<BrawlerHLSL::ModelInstanceLODMeshData> lodMeshDataBuffer = IMPL::Bindless_GlobalModelInstanceLODMeshDataBuffer[BINDLESS_MODEL_INSTANCE_LOD_MESH_DATA_BUFFER_INDEX];

			return lodMeshDataBuffer[NonUniformResourceIndex(modelInstanceID)];
		}

		BrawlerHLSL::ViewTransformData GetGlobalViewTransformData(in const uint viewID)
		{
			static const uint BINDLESS_VIEW_TRANSFORM_DATA_BUFFER_INDEX = 4;
			StructuredBuffer<BrawlerHLSL::ViewTransformData> viewTransformDataBuffer = IMPL::Bindless_GlobalViewTransformDataBuffer[BINDLESS_VIEW_TRANSFORM_DATA_BUFFER_INDEX];

			return viewTransformDataBuffer[viewID];
		}

		BrawlerHLSL::ViewDimensionsData GetGlobalViewDimensionsData(in const uint viewID)
		{
			static const uint BINDLESS_VIEW_DIMENSIONS_DATA_BUFFER_INDEX = 5;
			StructuredBuffer<BrawlerHLSL::ViewDimensionsData> viewDimensionsDataBuffer = IMPL::Bindless_GlobalViewDimensionsDataBuffer[BINDLESS_VIEW_DIMENSIONS_DATA_BUFFER_INDEX];

			return viewDimensionsDataBuffer[BINDLESS_VIEW_DIMENSIONS_DATA_BUFFER_INDEX];
		}

		BrawlerHLSL::PackedTriangleCluster GetGlobalPackedTriangleCluster(in const uint clusterID)
		{
			static const uint BINDLESS_GLOBAL_TRIANGLE_CLUSTER_BUFFER_INDEX = 6;
			StructuredBuffer<BrawlerHLSL::PackedTriangleCluster> triangleClusterBuffer = IMPL::Bindless_GlobalTriangleClusterBuffer[BINDLESS_GLOBAL_TRIANGLE_CLUSTER_BUFFER_INDEX];

			return triangleClusterBuffer[NonUniformResourceIndex(clusterID)];
		}

		BrawlerHLSL::GPUSceneState GetGlobalGPUSceneState()
		{
			static const uint BINDLESS_GLOBAL_GPU_SCENE_STATE_BUFFER_INDEX = 7;
			StructuredBuffer<BrawlerHLSL::GPUSceneState> sceneStateBuffer = IMPL::Bindless_GlobalGPUSceneStateBuffer[BINDLESS_GLOBAL_GPU_SCENE_STATE_BUFFER_INDEX];

			return sceneStateBuffer[0];
		}
	}
}