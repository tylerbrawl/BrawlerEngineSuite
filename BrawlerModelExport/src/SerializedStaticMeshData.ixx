module;
#include <cstdint>
#include <DirectXMath/DirectXMath.h>

export module Brawler.SerializedStaticMeshData;
import Brawler.SerializedMaterialDefinition;

export namespace Brawler
{
#pragma pack(push)
#pragma pack(1)
	struct SerializedStaticMeshData
	{
		DirectX::XMFLOAT3 AABBMinPoint;
		std::uint32_t VertexCount;

		DirectX::XMFLOAT3 AABBMaxPoint;
		std::uint32_t IndexCount;

		std::uint64_t VertexBufferFilePathHash;
		std::uint64_t IndexBufferFilePathHash;
	};
#pragma pack(pop)
}