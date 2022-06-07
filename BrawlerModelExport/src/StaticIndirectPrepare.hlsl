// We might also be able to use this exact same struct for skinned data, too! The difference there
// is that we would be creating a separate buffer containing the bone indices and weights.
struct StaticVertex
{
	float4 PositionAndTangentFrame;
	
	float2 UVCoords;
    uint MaterialIndex;
    uint __Pad0;
};

// NOTE: For Doom Eternal's dynamic draw call merging, the idea is *NOT* to make an actual index
// buffer, as the slides seem to suggest. Rather, one should dynamically create a vertex buffer for 
// a non-indexed draw. The vertices in this buffer contain only one attribute: an index into the
// global vertex StructuredBuffer containing the actual data for said vertex.
//
// id Software did something different, but unless there are some crazy hardware shenanigans which
// make my method less performant, it should work just as well.

RWBuffer<uint> CommandCountOutput : register(u1, space0);

[numthreads(64, 1, 1)]
void main(in const uint3 DTid : SV_DispatchThreadID)
{
}