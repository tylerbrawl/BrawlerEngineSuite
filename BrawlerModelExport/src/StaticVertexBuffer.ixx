module;
#include <vector>
#include <assimp/mesh.h>
#include <DirectXMath/DirectXMath.h>

export module Brawler.StaticVertexBuffer;
import Brawler.Math.AABB;

export namespace Brawler
{
	class StaticVertexBuffer
	{
	private:
		struct UnpackedStaticVertex
		{
			DirectX::XMFLOAT3 Position;
			DirectX::XMFLOAT3 Normal;
			DirectX::XMFLOAT3 Tangent;
			DirectX::XMFLOAT2 UVCoords;
		};

		struct PackedStaticVertex
		{
			DirectX::XMFLOAT4 PositionAndTangentFrame;
			DirectX::XMFLOAT2 UVCoords;
		};

	public:
		explicit StaticVertexBuffer(const aiMesh& mesh);

		StaticVertexBuffer(const StaticVertexBuffer& rhs) = delete;
		StaticVertexBuffer& operator=(const StaticVertexBuffer& rhs) = delete;

		StaticVertexBuffer(StaticVertexBuffer&& rhs) noexcept = default;
		StaticVertexBuffer& operator=(StaticVertexBuffer&& rhs) noexcept = default;

		void InitializePackedData();

	private:
		void InitializeUnpackedData(const aiMesh& mesh);

	private:
		std::vector<UnpackedStaticVertex> mUnpackedVertices;
		std::vector<PackedStaticVertex> mPackedVertices;
		Math::AABB mBoundingBox;
	};
}