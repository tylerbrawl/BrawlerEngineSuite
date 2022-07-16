module;
#include <cstdint>
#include <DirectXMath/DirectXMath.h>

export module Brawler.GPUSceneTypes;
import Util.HLSL;

export namespace Brawler
{
	struct PackedStaticVertex
	{
		DirectX::XMFLOAT4 PositionAndTangentFrame;

		DirectX::XMFLOAT2 UVCoords;
		DirectX::XMUINT2 __Pad0;
	};

	struct ModelInstanceTransformData
	{
		DirectX::XMFLOAT4X3 CurrentFrameWorldMatrix;
		DirectX::XMFLOAT4X3 CurrentFrameInverseWorldMatrix;

		DirectX::XMFLOAT4X3 PreviousFrameWorldMatrix;
		DirectX::XMFLOAT4X3 PreviousFrameInverseWorldMatrix;
	};

	struct LODMeshData
	{
		DirectX::XMFLOAT3 CurrentFrameAABBMin;
		std::uint32_t StartingTriangleClusterID;

		DirectX::XMFLOAT3 CurrentFrameAABBMax;
		std::uint32_t NumTriangleClusters;
	};

	struct ViewTransformData
	{
		DirectX::XMFLOAT4X4 CurrentFrameViewProjectionMatrix;
		DirectX::XMFLOAT4X4 CurrentFrameInverseViewProjectionMatrix;

		DirectX::XMFLOAT4X4 PreviousFrameViewProjectionMatrix;
		DirectX::XMFLOAT4X4 PreviousFrameInverseViewProjectionMatrix;

		// We can save on memory by realizing that the view matrix is an orthogonal matrix.
		// Since every orthogonal matrix represents a unique rotation, we can convert our
		// view matrices into quaternions and store those, instead.
		//
		// We need to add them at the very end of the structure for the sake of alignment.
		// Thankfully, the Brawler Engine uses reflection to throw a compile-time error if
		// it detects that a type is not properly defined/aligned for transfer to the GPU.

		DirectX::XMFLOAT4 CurrentFrameViewSpaceQuaternion;
		DirectX::XMFLOAT4 PreviousFrameViewSpaceQuaternion;
	};

	struct ViewDimensionsData
	{
		DirectX::XMUINT2 ViewDimensions;
		DirectX::XMFLOAT2 InverseViewDimensions;
	};

	struct PackedTriangleCluster
	{
		DirectX::XMFLOAT3 CurrentFrameAABBMin;
		std::uint32_t MaterialDefinitionIndexAndBiasedTriangleCount;

		DirectX::XMFLOAT3 CurrentFrameAABBMax;
		std::uint32_t StartingIndexBufferIndex;
	};
}