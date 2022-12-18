#include "DeferredGeometryRasterTypes.hlsli"
#include "BindlessDescriptors.hlsli"
#include "MeshTypes.hlsli"
#include "ViewTypes.hlsli"
#include "NormalPacking.hlsli"
#include "TransformUtil.hlsli"
#include "MathConstants.hlsli"

BrawlerHLSL::DeferredGeometryRasterVSOutput main(in const BrawlerHLSL::DeferredGeometryRasterVSInput inputVertex)
{
	// TODO: This is a very busy vertex shader... Well, I suppose that's better than
	// a very busy pixel shader, but still...
	
	const uint meshDescriptorBufferIndex = (inputVertex.PackedModelInstanceDescriptorBufferIndexAndMeshDescriptorBufferIndex & 0xFFFF);
	const uint modelInstanceDescriptorBufferIndex = (inputVertex.PackedModelInstanceDescriptorBufferIndexAndMeshDescriptorBufferIndex >> 16);
	
	const BrawlerHLSL::PackedStaticVertex currVertex = BrawlerHLSL::GetGlobalVertexBufferVertex(inputVertex.GlobalVertexBufferIndex);
	const BrawlerHLSL::ModelInstanceDescriptor currModelInstanceDescriptor = BrawlerHLSL::GetGlobalModelInstanceDescriptor(modelInstanceDescriptorBufferIndex);
			
	StructuredBuffer<BrawlerHLSL::MeshDescriptor> currModelMeshDescriptorBuffer = BrawlerHLSL::GetGlobalMeshDescriptorBuffer(currModelInstanceDescriptor.MeshDescriptorBufferID);
	const BrawlerHLSL::MeshDescriptor currMeshDescriptor = currModelMeshDescriptorBuffer[NonUniformResourceIndex(meshDescriptorBufferIndex)];
	
	const BrawlerHLSL::ModelInstanceTransformData currTransformData = BrawlerHLSL::GetGlobalModelInstanceTransformData(currModelInstanceDescriptor.TransformDataBufferIndex);
	
	const BrawlerHLSL::ViewDescriptor currViewDescriptor = WaveReadLaneFirst(BrawlerHLSL::GetGlobalViewDescriptor(DeferredGeometryRasterConstants.ViewID));
	const BrawlerHLSL::ViewTransformData currViewTransformData = WaveReadLaneFirst(BrawlerHLSL::GetGlobalViewTransformData(currViewDescriptor.ViewTransformBufferIndex));
	
	const float4x4 worldMatrix = Util::Transform::ExpandWorldMatrix(currTransformData.CurrentFrameWorldMatrix);
	
	const float4 currPositionWS = mul(float4(currVertex.PositionAndTangentFrame.xyz, 1.0f), worldMatrix);
	const float4 currPositionCS = mul(currPositionWS, currViewTransformData.CurrentFrameViewProjectionMatrix);

	// From "Introduction to 3D Game Programming with Direct X 12":
	//
	// The strictly correct way to transform vectors such that they remain orthogonal
	// to the plane they were normal to before transformation is to use the inverse-transpose
	// of any matrix with nonuniform scaling:
	//
	// n' = mul(n, Inverse(Transpose(WorldMatrix)));
	//
	// Since we already have access to the inverse world matrix from currTransformData, we
	// might as well do this correctly. Keep in mind, however, that we should explicitly
	// zero any elements which might have come from a translation.
	float4x4 inverseTransposeWorldMatrix = Util::Math::ExpandWorldMatrix(currTransformData.CurrentFrameInverseWorldMatrix);
	inverseTransposeWorldMatrix[3] = float4(0.0f, 0.0f, 0.0f, 1.0f);
	inverseTransposeWorldMatrix = transpose(inverseTransposeWorldMatrix);
	
	const float4x4 invTransposeWorldViewProjectionMatrix = mul(inverseTransposeWorldMatrix, currViewTransformData.CurrentFrameViewProjectionMatrix);
	
	// These might just be the crustiest vertex normals ever conceived.
	const uint reallyPackedTangentFrame = asuint(currVertex.PositionAndTangentFrame.w);

	static const float ENCODED_NORMAL_SCALE_VALUE = 0.00392157f;  // Mapping [0, 255] to [0, 1]: y = (1/255)x
	static const float ROTATION_SCALE_VALUE = 0.02463994f;  // Mapping [0, 255] to [0, 2Pi]: y = (2Pi/255)x
	
	float3 packedTangentFrame = float3(
		float(reallyPackedTangentFrame >> 24) * ENCODED_NORMAL_SCALE_VALUE,  // x stores the encoded x component of the normal.
		float((reallyPackedTangentFrame >> 16) & 0xFF) * ENCODED_NORMAL_SCALE_VALUE,  // y stores the encoded y component of the normal.
		float((reallyPackedTangentFrame >> 8) & 0xFF) * ROTATION_SCALE_VALUE  // z stores the rotation, in radians, of tangent_b about the normal to get the tangent.
	);
	
	// Finish the transformation from [0, 255] to [-1, 1].
	packedTangentFrame.z -= 1.0f;
	
	const float4x4 tangentFrameOS = UnpackTangentFrame(packedTangentFrame);
	const float4x4 tangentFrameWS = mul(tangentFrameOS, inverseTransposeWorldMatrix);
	
	BrawlerHLSL::DeferredGeometryRasterVSOutput output;
	output.PositionCS = currPositionCS.xyz;
	output.PositionWS = currPositionWS.xyz;
	output.NormalWS = normalize(tangentFrameWS[2]);
	output.TangentWS = normalize(tangentFrameWS[0]);
	output.BitangentWS = normalize(tangentFrameWS[1]);
	output.UVCoords = currVertex.UVCoords;
	output.MaterialDescriptorBufferIndex = currMeshDescriptor.MaterialDescriptorIndex;
			
	return output;
}