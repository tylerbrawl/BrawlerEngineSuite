module;
#include <cstdint>
#include <DirectXMath/DirectXMath.h>

export module Brawler.GPUSceneTypes;
import Util.HLSL;

// NOTE: For some of these types, you will see that their corresponding HLSL structure has bit packing,
// while their C++ types combine them into a single member. The reflection used by the Brawler Engine
// cannot correctly infer the sizes of members with bit packing, so we don't use it on this end.

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

		DirectX::XMFLOAT4 CurrentFrameViewSpaceQuaternion;

		DirectX::XMFLOAT3 CurrentFrameViewSpaceOrigin;
		std::uint32_t __Pad0;

		DirectX::XMFLOAT4 PreviousFrameViewSpaceQuaternion;

		DirectX::XMFLOAT3 PreviousFrameViewSpaceOrigin;
		std::uint32_t __Pad1;
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

	struct VirtualTextureDescription
	{
		std::uint32_t IndirectionTextureIndexAndLog2VTSize;
		std::uint32_t CombinedPageXAndYCoord;
		std::uint32_t GlobalTextureIndex;
		std::uint32_t __Pad0;
	};
}