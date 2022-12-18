// NOTE: The octahedron normal encoding/decoding algorithm was shamelessly
// copied from https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/.
// It explicitly states that the code can be copied into our own engines.
// Thanks, Krzysztof Narkowicz!

float2 OctahedronWrap(in const float2 v)
{
    return ((1.0f - abs(v.yx)) * select(v.xy >= 0.0f, 1.0f, -1.0f));
}

float2 PackNormal(in float3 unpackedNormal)
{
    unpackedNormal /= (abs(unpackedNormal.x) + abs(unpackedNormal.y) + abs(unpackedNormal.z));
    
    if (unpackedNormal.z < 0.0f)
        unpackedNormal.xy = OctahedronWrap(unpackedNormal.xy);

    // Encode the values in the range [0, 1].
    unpackedNormal.xy = (unpackedNormal.xy * 0.5f) + 0.5f;

    return unpackedNormal.xy;
}

float3 UnpackNormal(in float2 packedNormal)
{
    // Re-map the values into the range [-1, 1], since these are unit
    // vectors.
    packedNormal = (packedNormal * 2.0f) - 1.0f;

    float3 normal = float3(packedNormal.x, packedNormal.y, (1.0f - abs(packedNormal.x) - abs(packedNormal.y)));

    float t = saturate(-normal.z);
    normal.xy += (normal.xy >= 0.0f ? -t : t);

    return normalize(normal);
}