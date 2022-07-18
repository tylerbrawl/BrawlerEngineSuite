module;
#include <vector>
#include <span>
#include <assimp/mesh.h>

export module Brawler.TriangleCluster;
import Brawler.Math.AABB;
import Brawler.ImportedMesh;

export namespace Brawler
{
	class TriangleCluster
	{
	public:
		TriangleCluster() = default;
		explicit TriangleCluster(const ImportedMesh& mesh);

		TriangleCluster(const TriangleCluster& rhs) = delete;
		TriangleCluster& operator=(const TriangleCluster& rhs) = delete;

		TriangleCluster(TriangleCluster&& rhs) noexcept = default;
		TriangleCluster& operator=(TriangleCluster&& rhs) noexcept = default;

		void AddTriangle(const aiFace& triangleFace);

		std::span<const std::uint32_t> GetIndexSpan() const;
		Math::AABB CalculateBoundingBox() const;

		std::size_t GetTriangleCount() const;
		bool HasTriangles() const;

		static consteval std::size_t GetMaximumTriangleCount();

	private:
		const ImportedMesh* mMeshPtr;
		std::vector<std::uint32_t> mIndexArr;
	};
}

// -------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	consteval std::size_t TriangleCluster::GetMaximumTriangleCount()
	{
		constexpr std::size_t MAX_TRIANGLES_PER_CLUSTER = 128;
		return MAX_TRIANGLES_PER_CLUSTER;
	}
}