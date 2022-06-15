module;
#include <vector>
#include <span>
#include <DirectXMath/DirectXMath.h>

export module Brawler.StaticVertexBuffer;
import Brawler.Math.AABB;
import Brawler.StaticVertexData;
import Brawler.FilePathHash;
import Brawler.ImportedMesh;

export namespace Brawler
{
	class StaticVertexBuffer
	{
	public:
		explicit StaticVertexBuffer(const ImportedMesh& mesh);

		StaticVertexBuffer(const StaticVertexBuffer& rhs) = delete;
		StaticVertexBuffer& operator=(const StaticVertexBuffer& rhs) = delete;

		StaticVertexBuffer(StaticVertexBuffer&& rhs) noexcept = default;
		StaticVertexBuffer& operator=(StaticVertexBuffer&& rhs) noexcept = default;

		void Update();
		bool IsReadyForSerialization() const;

		FilePathHash SerializeVertexBuffer() const;

		std::span<const UnpackedStaticVertex> GetUnpackedVertexSpan() const;

		const Math::AABB& GetBoundingBox() const;
		std::size_t GetVertexCount() const;

	private:
		void InitializePackedData();
		void InitializeUnpackedData(const aiMesh& mesh);

	private:
		std::vector<UnpackedStaticVertex> mUnpackedVertices;
		std::vector<PackedStaticVertex> mPackedVertices;
		Math::AABB mBoundingBox;
		const ImportedMesh* mMeshPtr;
	};
}