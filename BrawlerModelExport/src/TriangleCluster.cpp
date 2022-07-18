module;
#include <vector>
#include <cassert>
#include <span>
#include <ranges>
#include <DirectXMath/DirectXMath.h>
#include <assimp/mesh.h>

module Brawler.TriangleCluster;

namespace Brawler
{
	TriangleCluster::TriangleCluster(const ImportedMesh& mesh) :
		mMeshPtr(&mesh),
		mIndexArr()
	{
		// Anticipate mIndexArr holding (128 * 3) indices, i.e., that this TriangleCluster instance
		// will indeed contain the maximum number of triangles. This isn't always the case, but it
		// is common enough.
		static constexpr std::size_t MAX_INDICES_PER_CLUSTER = (TriangleCluster::GetMaximumTriangleCount() * 3);
		mIndexArr.reserve(MAX_INDICES_PER_CLUSTER);
	}

	void TriangleCluster::AddTriangle(const aiFace& triangleFace)
	{
		assert(triangleFace.mNumIndices == 3 && "ERROR: An attempt was made to add an aiFace instance which was not a triangle to a TriangleCluster!");
		assert(GetTriangleCount() < TriangleCluster::GetMaximumTriangleCount() && "ERROR: An attempt was made to add more triangles to a TriangleCluster than is allowed!");

		mIndexArr.push_back(triangleFace.mIndices[0]);
		mIndexArr.push_back(triangleFace.mIndices[1]);
		mIndexArr.push_back(triangleFace.mIndices[2]);
	}

	std::span<const std::uint32_t> TriangleCluster::GetIndexSpan() const
	{
		return std::span<const std::uint32_t>{ mIndexArr };
	}

	Math::AABB TriangleCluster::CalculateBoundingBox() const
	{
		static constexpr DirectX::XMFLOAT3 STARTING_MIN_POINT{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
		static constexpr DirectX::XMFLOAT3 STARTING_MAX_POINT{ std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };
		
		assert(mMeshPtr != nullptr);

		Math::AABB boundingBox{ DirectX::XMFLOAT3{ STARTING_MIN_POINT }, DirectX::XMFLOAT3{ STARTING_MAX_POINT } };
		const aiMesh& aiMesh{ mMeshPtr->GetMesh() };
		const std::span<const aiVector3D> vertexSpan{ aiMesh.mVertices, static_cast<std::size_t>(aiMesh.mNumVertices) };

		// Add in each vertex position into the AABB.
		const std::size_t numTriangles = GetTriangleCount();
		for (const std::size_t i : std::views::iota(0u, numTriangles))
		{
			const std::size_t baseIndexBufferOffset = (i * 3);

			const aiVector3D& triangleVertexA{ vertexSpan[mIndexArr[baseIndexBufferOffset]] };
			const DirectX::XMFLOAT3 vertexAPos{ triangleVertexA.x, triangleVertexA.y, triangleVertexA.z };
			boundingBox.InsertPoint(DirectX::XMLoadFloat3(&vertexAPos));

			const aiVector3D& triangleVertexB{ vertexSpan[mIndexArr[baseIndexBufferOffset + 1]] };
			const DirectX::XMFLOAT3 vertexBPos{ triangleVertexB.x, triangleVertexB.y, triangleVertexB.z };
			boundingBox.InsertPoint(DirectX::XMLoadFloat3(&vertexBPos));

			const aiVector3D& triangleVertexC{ vertexSpan[mIndexArr[baseIndexBufferOffset + 2]] };
			const DirectX::XMFLOAT3 vertexCPos{ triangleVertexC.x, triangleVertexC.y, triangleVertexC.z };
			boundingBox.InsertPoint(DirectX::XMLoadFloat3(&vertexCPos));
		}

		return boundingBox;
	}

	std::size_t TriangleCluster::GetTriangleCount() const
	{
		return (mIndexArr.size() / 3);
	}

	bool TriangleCluster::HasTriangles() const
	{
		assert(mIndexArr.size() % 3 == 0);
		return !mIndexArr.empty();
	}
}