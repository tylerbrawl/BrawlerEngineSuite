module;
#include <span>
#include <cassert>
#include <ranges>
#include <cmath>
#include <vector>
#include <DirectXMath/DirectXMath.h>

export module Brawler.NormalBoundingConeSolver;
import Util.General;
import Brawler.NormalBoundingCone;

// NOTE: The implementation of this type is heavily inspired by the academic whitepaper
// "Optimal bounding cones of vectors in three dimensions" by Gill Barequet and
// Gershon Elber.

namespace Brawler
{
	template <typename Vertex>
	concept HasPosition = requires (const Vertex& v)
	{
		{ v.GetPosition() } -> std::same_as<const DirectX::XMFLOAT3&>;
	};
}

namespace Brawler
{
	static constexpr float EPSILON = 0.01f;

	struct SphericalCircle
	{
		DirectX::XMFLOAT3 CenterPoint;
		float ConeAngle;

		bool IsPointWithinCircle(const DirectX::XMFLOAT3& point) const
		{
			const float angleBetweenCenterAndPoint = DirectX::XMVectorGetX(DirectX::XMVector3AngleBetweenNormals(DirectX::XMLoadFloat3(&CenterPoint), DirectX::XMLoadFloat3(&point)));
			return (angleBetweenCenterAndPoint + EPSILON <= ConeAngle);
		}
	};
}

export namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	class NormalBoundingConeSolver
	{
	public:
		NormalBoundingConeSolver() = default;

		NormalBoundingConeSolver(const NormalBoundingConeSolver& rhs) = delete;
		NormalBoundingConeSolver& operator=(const NormalBoundingConeSolver& rhs) = delete;

		NormalBoundingConeSolver(NormalBoundingConeSolver&& rhs) noexcept = default;
		NormalBoundingConeSolver& operator=(NormalBoundingConeSolver&& rhs) noexcept = default;

		void SetVertexSpan(const std::span<const Vertex> vertexSpan);
		NormalBoundingCone CalculateNormalBoundingCone(const std::span<const std::uint16_t> indexSpan);

	private:
		void AddNewPoint(const DirectX::XMFLOAT3& point);
		void AdjustSphericalCircleForPoint(const DirectX::XMFLOAT3& point);

	private:
		std::span<const Vertex> mVertexSpan;
		SphericalCircle mCircle;
		std::vector<DirectX::XMFLOAT3> mProcessedPointArr;
	};
}

// ----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	void AssertVectorNormalization(const DirectX::XMFLOAT3& vector);

	template <typename Vertex>
		requires HasPosition<Vertex>
	SphericalCircle GetAdjustedSphericalCircleForTwoPoints(const std::span<const DirectX::XMFLOAT3> processedPointSpan, const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB);

	SphericalCircle GetEncompassingSphericalCircle(const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB);
	SphericalCircle GetEncompassingSphericalCircle(const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB, const DirectX::XMFLOAT3& pointC);
}

namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	SphericalCircle GetAdjustedSphericalCircleForTwoPoints(const std::span<const DirectX::XMFLOAT3> processedPointSpan, const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB)
	{
		SphericalCircle adjustedSphericalCircle = GetEncompassingSphericalCircle(pointA, pointB);

		for (const auto& currPoint : processedPointSpan)
		{
			if (!adjustedSphericalCircle.IsPointWithinCircle(currPoint))
				adjustedSphericalCircle = GetEncompassingSphericalCircle(pointA, pointB, currPoint);
		}

		return adjustedSphericalCircle;
	}
}

namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	void NormalBoundingConeSolver<Vertex>::SetVertexSpan(const std::span<const Vertex> vertexSpan)
	{
		mVertexSpan = vertexSpan;
	}
	
	template <typename Vertex>
		requires HasPosition<Vertex>
	NormalBoundingCone NormalBoundingConeSolver<Vertex>::CalculateNormalBoundingCone(const std::span<const std::uint16_t> indexSpan)
	{
		assert(!mVertexSpan.empty());
		assert(indexSpan.size() >= 3 && indexSpan.size() % 3 == 0);

		// Reset the current SphericalCircle instance and mProcessedPointArr.
		mCircle = SphericalCircle{};
		mProcessedPointArr.clear();

		mProcessedPointArr.reserve(indexSpan.size() / 3);

		// It is important to note that we need to create the normal bounding cone for
		// *triangles,* and *NOT* for vertices.

		for (std::size_t i = 0; i < indexSpan.size(); i += 3)
		{
			// Re-construct the normal of the triangle formed by the three relevant indices. We let
			// Assimp convert the mesh to our left-handed format. Let A, B, and C be indices in the
			// indexSpan, and suppose that they are listed in the order A, B, and then C. The normal
			// we want can then be constructed from AB x BC.
			assert(indexSpan[i] < mVertexSpan.size() && indexSpan[i + 1] < mVertexSpan.size() && indexSpan[i + 2] < mVertexSpan.size() && 
				"ERROR: An out-of-bounds index was detected in the std::span of indices provided to NormalBoundingConeSolver::CalculateNormalBoundingCone()!");

			const Vertex& vertexA{ mVertexSpan[indexSpan[i]] };
			const DirectX::XMVECTOR positionA{ DirectX::XMLoadFloat3(&(vertexA.GetPosition())) };

			const Vertex& vertexB{ mVertexSpan[indexSpan[i + 1]] };
			const DirectX::XMVECTOR positionB{ DirectX::XMLoadFloat3(&(vertexB.GetPosition())) };

			const Vertex& vertexC{ mVertexSpan[indexSpan[i + 2]] };
			const DirectX::XMVECTOR positionC{ DirectX::XMLoadFloat3(&(vertexC.GetPosition())) };

			const DirectX::XMVECTOR triangleNormal{ DirectX::XMVector3Normalize(DirectX::XMVector3Cross(DirectX::XMVectorSubtract(positionB, positionA), DirectX::XMVectorSubtract(positionC, positionB))) };

			DirectX::XMFLOAT3 storedTriangleNormal{};
			DirectX::XMStoreFloat3(&storedTriangleNormal, triangleNormal);

			AddNewPoint(storedTriangleNormal);
		}

		return NormalBoundingCone{
			.ConeNormal{ mCircle.CenterPoint },

			// It is recommended in "Optimizing the Graphics Pipeline with Compute" that the angle
			// be expanded slightly in order to not reject valid normals during culling.
			.NegativeSineAngle{ -std::sin(mCircle.ConeAngle + EPSILON) }
		};
	}

	template <typename Vertex>
		requires HasPosition<Vertex>
	void NormalBoundingConeSolver<Vertex>::AddNewPoint(const DirectX::XMFLOAT3& point)
	{
		if (mProcessedPointArr.empty()) [[unlikely]]
		{
			// Set the center point to be that of the first normal we find.
			mCircle.CenterPoint = point;
		}

		else if (mProcessedPointArr.size() == 1) [[unlikely]]
		{
			// The second processed point should be used to construct the first real spherical circle.
			mCircle = GetEncompassingSphericalCircle(mCircle.CenterPoint, point);
		}

		else if (!mCircle.IsPointWithinCircle(point))
			AdjustSphericalCircleForPoint(point);

		mProcessedPointArr.push_back(point);
	}

	template <typename Vertex>
		requires HasPosition<Vertex>
	void NormalBoundingConeSolver<Vertex>::AdjustSphericalCircleForPoint(const DirectX::XMFLOAT3& point)
	{
		assert(!mProcessedPointArr.empty());

		SphericalCircle adjustedSphericalCircle{ GetEncompassingSphericalCircle(mProcessedPointArr[0], point)};

		for (const auto i : std::views::iota(1u, mProcessedPointArr.size()))
		{
			// We don't need to continue asserting that the vectors are normalized if we already do
			// so in NormalBoundingConeSolver::CalculateNormalBoundingCone().

			const DirectX::XMFLOAT3& currNormal{ mProcessedPointArr[i] };

			if (!adjustedSphericalCircle.IsPointWithinCircle(currNormal))
				adjustedSphericalCircle = GetAdjustedSphericalCircleForTwoPoints(std::span<const DirectX::XMFLOAT3>{ mProcessedPointArr | std::views::take(i) }, currNormal, point);
		}

		mCircle = std::move(adjustedSphericalCircle);
	}
}