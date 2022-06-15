module;
#include <span>
#include <cassert>
#include <ranges>
#include <cmath>
#include <vector>
#include <DirectXMath/DirectXMath.h>

export module Brawler.NormalBoundingCones:NormalBoundingConeSolver;
import Util.General;
import :NormalBoundingConeTypes;

// NOTE: The implementation of this type is heavily inspired by the academic whitepaper
// "Optimal bounding cones of vectors in three dimensions" by Gill Barequet and
// Gershon Elber.

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

		NormalBoundingCone CalculateNormalBoundingCone(const std::span<const Triangle<Vertex>> triangleSpan);

	private:
		void AddNewPoint(const DirectX::XMFLOAT3& point);
		void AdjustSphericalCircleForPoint(const DirectX::XMFLOAT3& point);

	private:
		SphericalCircle mCircle;
		std::vector<DirectX::XMFLOAT3> mProcessedPointArr;
	};
}

// ----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	void AssertVectorNormalization(const DirectX::XMFLOAT3& vector);

	SphericalCircle GetAdjustedSphericalCircleForTwoPoints(const std::span<const DirectX::XMFLOAT3> processedPointSpan, const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB);

	SphericalCircle GetEncompassingSphericalCircle(const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB);
	SphericalCircle GetEncompassingSphericalCircle(const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB, const DirectX::XMFLOAT3& pointC);
}

namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	NormalBoundingCone NormalBoundingConeSolver<Vertex>::CalculateNormalBoundingCone(const std::span<const Triangle<Vertex>> triangleSpan)
	{
		assert(!triangleSpan.empty());

		// Reset the current SphericalCircle instance and mProcessedPointArr.
		mCircle = SphericalCircle{};
		mProcessedPointArr.clear();

		mProcessedPointArr.reserve(triangleSpan.size());

		// It is important to note that we need to create the normal bounding cone for
		// *triangles,* and *NOT* for vertices.

		for (const auto& triangle : triangleSpan)
			AddNewPoint(triangle.GetTriangleNormal());

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