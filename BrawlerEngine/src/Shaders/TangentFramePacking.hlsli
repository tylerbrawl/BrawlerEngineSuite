#include "NormalPacking.hlsli"
#include "MathConstants.hlsli"

// Given a packed tangent frame, this function creates the corresponding
// tangent frame matrix. For more information, see the SIGGRAPH 2020 
// presentation "Rendering the Hellscape of Doom: Eternal."
float3x3 UnpackTangentFrame3x3(in const float3 packedTangentFrame)
{
    const float3 normal = UnpackNormal(packedTangentFrame.xy);
    const float rotationRadians = packedTangentFrame.z;

    const float3 tangent_b = (abs(normal.x) > abs(normal.z) ?
        float3(-normal.y, normal.x, 0.0f) :
        float3(0.0f, -normal.z, normal.y)
    );
    
	const float cosineRotation = cos(rotationRadians);
    
	float sineRotation = sqrt(1.0f - (cosineRotation * cosineRotation));
	sineRotation *= (rotationRadians > BrawlerHLSL::PI && rotationRadians < BrawlerHLSL::TWO_PI ? -1.0f : 1.0f);

    // One way to do this would be to use a quaternion, but we can actually
    // just use the traditional Rodrigues rotation equation here.
	const float3 tangent = normalize(tangent_b * cosineRotation + cross(normal, tangent_b) * sineRotation);

    const float3 bitangent = cross(normal, tangent);

    return float3x3(tangent, bitangent, normal);
}

float4x4 UnpackTangentFrame(in const float3 packedTangentFrame)
{
    const float3x3 tangentFrame = UnpackTangentFrame3x3(packedTangentFrame);
    return float4x4(
        float4(tangentFrame[0], 0.0f),
        float4(tangentFrame[1], 0.0f),
        float4(tangentFrame[2], 0.0f),
        float4(0.0f, 0.0f, 0.0f, 1.0f)
    );
}