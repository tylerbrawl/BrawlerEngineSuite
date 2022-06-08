module;
#include <vector>
#include <span>
#include <assimp/mesh.h>
#include <DirectXMath/DirectXMath.h>

export module Brawler.StaticVertexBuffer;
import Brawler.Math.AABB;
import Brawler.StaticVertexData;

export namespace Brawler
{
	class StaticVertexBuffer
	{
	public:
		explicit StaticVertexBuffer(const aiMesh& mesh);

		StaticVertexBuffer(const StaticVertexBuffer& rhs) = delete;
		StaticVertexBuffer& operator=(const StaticVertexBuffer& rhs) = delete;

		StaticVertexBuffer(StaticVertexBuffer&& rhs) noexcept = default;
		StaticVertexBuffer& operator=(StaticVertexBuffer&& rhs) noexcept = default;

		void Update();
		bool IsReadyForSerialization() const;

		std::span<const UnpackedStaticVertex> GetUnpackedVertexSpan() const;

	private:
		void InitializePackedData();
		void InitializeUnpackedData(const aiMesh& mesh);

	private:
		std::vector<UnpackedStaticVertex> mUnpackedVertices;
		std::vector<PackedStaticVertex> mPackedVertices;
		Math::AABB mBoundingBox;
	};
}