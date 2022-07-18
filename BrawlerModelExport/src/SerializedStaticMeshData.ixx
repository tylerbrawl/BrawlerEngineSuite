module;
#include <cstdint>
#include <DirectXMath/DirectXMath.h>

export module Brawler.SerializedStaticMeshData;
import Brawler.IndexBufferHeader;

export namespace Brawler
{
#pragma pack(push)
#pragma pack(1)
	struct SerializedStaticMeshData
	{
		DirectX::XMFLOAT3 AABBMinPoint;
		DirectX::XMFLOAT3 AABBMaxPoint;

		std::uint64_t VertexBufferFilePathHash;
		std::uint64_t IndexBufferFilePathHash;

		SerializedIndexBufferHeader IndexBufferInfo;
		std::uint32_t VertexCount;
	};
#pragma pack(pop)
}