// We might also be able to use this exact same struct for skinned data, too! The difference there
// is that we would be creating a separate buffer containing the bone indices and weights.
struct StaticVertex
{
    float4 PositionAndTangentFrame;
	
    float2 UVCoords;
    uint2 __Pad0;
};

// NOTE: *ALL* matrices are made implicitly row-major by the Brawler Shader Compiler.

struct ModelInstanceData
{
    float4x4 WorldMatrix;
};

static const uint MAX_MODEL_INSTANCES = 65536;

// The triangle cluster system was ripped wholesale from the awesome UE5 Nanite presentation
// given at Advances in Real-Time Rendering 2021. Each cluster has at most 128 triangles, and
// there is a maximum of 2^25 triangle clusters supported.

struct PackedTriangleCluster
{
    // If we limit ourselves to 2^16 model instances and each cluster has at most 128 (2^7)
    // triangles, then we can pack the ModelInstanceData index and cluster triangle count into a
    // single uint.
    uint ModelInstanceDataIndexAndTriangleCount;
    
    uint MaterialDefinitionIndex;
};

struct UnpackedTriangleCluster
{
    uint ModelInstanceDataIndex;
    uint TriangleCount;
    uint MaterialDefinitionIndex;
};

UnpackedTriangleCluster UnpackTriangleCluster(in const PackedTriangleCluster packedCluster)
{
    UnpackedTriangleCluster unpackedCluster;
    unpackedCluster.ModelInstanceDataIndex = (packedCluster.ModelInstanceDataIndexAndTriangleCount & 0xFFFF);
    unpackedCluster.TriangleCount = (packedCluster.ModelInstanceDataIndexAndTriangleCount >> 16);
    unpackedCluster.MaterialDefinitionIndex = packedCluster.MaterialDefinitionIndex;
    
    return unpackedCluster;
}

PackedTriangleCluster PackTriangleCluster(in const UnpackedTriangleCluster unpackedCluster)
{
    PackedTriangleCluster packedCluster;
    packedCluster.ModelInstanceDataIndexAndTriangleCount = ((unpackedCluster.TriangleCount << 16) | unpackedCluster.ModelInstanceDataIndex);
    packedCluster.MaterialDefinitionIndex = unpackedCluster.MaterialDefinitionIndex;
    
    return packedCluster;
}

static const uint MAX_TRIANGLES_PER_TRIANGLE_CLUSTER = 128;
static const uint MAX_TRIANGLE_CLUSTERS = (1 << 25);

struct ModelInstanceDataBuffer
{
    ModelInstanceData Entry[MAX_MODEL_INSTANCES];
};

struct TriangleClusterBuffer
{
    PackedTriangleCluster ClusterArr[MAX_TRIANGLE_CLUSTERS];
};