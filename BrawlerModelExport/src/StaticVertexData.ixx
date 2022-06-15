module;
#include <DirectXMath/DirectXMath.h>

export module Brawler.StaticVertexData;

export namespace Brawler
{
	struct UnpackedStaticVertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT3 Tangent;
		DirectX::XMFLOAT2 UVCoords;

		const DirectX::XMFLOAT3& GetPosition() const
		{
			return Position;
		}
	};

	struct PackedStaticVertex
	{
		// If we want to upload this data directly to the GPU as a StructuredBuffer
		// (and we do), then we need to pad it out like one.

		DirectX::XMFLOAT4 PositionAndTangentFrame;

		DirectX::XMFLOAT2 UVCoords;
		DirectX::XMUINT2 __Pad0;
	};
}

namespace Brawler
{
#pragma pack(push)
#pragma pack(1)
	struct SerializableStaticVertex
	{
		DirectX::XMFLOAT4 PositionAndTangentFrame;

		DirectX::XMFLOAT2 UVCoords;
		DirectX::XMUINT2 __Pad0;
	};
#pragma pack(pop)

	static_assert(sizeof(SerializableStaticVertex) == sizeof(PackedStaticVertex));
}