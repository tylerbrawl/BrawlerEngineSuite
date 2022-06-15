#include "GeometryTypes.hlsli"

// ================================================================================
// Bindless Types
//
// NOTE: By tradition, spaces greater than space0 are reserved for bindless SRV
// declarations.
// ================================================================================

StructuredBuffer<StaticVertex> Bindless_GlobalVertexBuffer[] : register(t0, space1);
Buffer<uint> Bindless_GlobalIndexBuffer[] : register(t0, space2);
StructuredBuffer<ModelInstanceDataBuffer> Bindless_GlobalModelInstanceDataBuffer[] : register(t0, space3);
StructuredBuffer<TriangleClusterBuffer> Bindless_GlobalTriangleClusterBuffer[] : register(t0, space4);

StructuredBuffer<StaticVertex> GetGlobalVertexBuffer()
{
    static const uint GLOBAL_VERTEX_BUFFER_BINDLESS_INDEX = 0;
	
    return Bindless_GlobalVertexBuffer[GLOBAL_VERTEX_BUFFER_BINDLESS_INDEX];
}

Buffer<uint> GetGlobalIndexBuffer()
{
    static const uint GLOBAL_INDEX_BUFFER_BINDLESS_INDEX = 1;
	
    return Bindless_GlobalIndexBuffer[GLOBAL_INDEX_BUFFER_BINDLESS_INDEX];
}

StructuredBuffer<ModelInstanceDataBuffer> GetModelInstanceDataBuffer()
{
    static const uint GLOBAL_MODEL_INSTANCE_DATA_BUFFER_BINDLESS_INDEX = 2;
	
    return Bindless_GlobalModelInstanceDataBuffer[GLOBAL_MODEL_INSTANCE_DATA_BUFFER_BINDLESS_INDEX];
}

StructuredBuffer<TriangleClusterBuffer> GetTriangleClusterBuffer()
{
    static const uint GLOBAL_TRIANGLE_CLUSTER_BUFFER_BINDLESS_INDEX = 3;
	
    return Bindless_GlobalTriangleClusterBuffer[GLOBAL_TRIANGLE_CLUSTER_BUFFER_BINDLESS_INDEX];
}