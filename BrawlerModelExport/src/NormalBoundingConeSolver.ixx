module;
#include <span>
#include <cassert>
#include <ranges>
#include <cmath>
#include <DirectXMath/DirectXMath.h>

export module Brawler.NormalBoundingConeSolver;
import Util.General;

// NOTE: The implementation of this type is heavily inspired by the academic whitepaper
// "Optimal bounding cones of vectors in three dimensions" by Gill Barequet and
// Gershon Elber.

namespace Brawler
{
	template <typename Vertex>
	concept HasNormal = requires (const Vertex& v)
	{
		{ v.GetNormal(); } -> std::same_as<const DirectX::XMFLOAT3&>;
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
		requires HasNormal<Vertex>
	class NormalBoundingConeSolver
	{
	public:
		NormalBoundingConeSolver() = default;

		NormalBoundingConeSolver(const NormalBoundingConeSolver& rhs) = delete;
		NormalBoundingConeSolver& operator=(const NormalBoundingConeSolver& rhs) = delete;

		NormalBoundingConeSolver(NormalBoundingConeSolver&& rhs) noexcept = default;
		NormalBoundingConeSolver& operator=(NormalBoundingConeSolver&& rhs) noexcept = default;

		void CalculateNormalBoundingCone(const std::span<const Vertex> vertexSpan);

		const DirectX::XMFLOAT3 GetConeNormal() const;
		float GetNegativeSineAngle() const;

	private:
		void AdjustSphericalCircleForPoint(const std::span<const Vertex> processedVertexSpan, const DirectX::XMFLOAT3& point);

	private:
		SphericalCircle mCircle;
	};
}

// ----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	void AssertVectorNormalization(const DirectX::XMFLOAT3& vector);

	template <typename Vertex>
		requires HasNormal<Vertex>
	SphericalCircle GetAdjustedSphericalCircleForTwoPoints(const std::span<const Vertex> processedVertexSpan, const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB);

	SphericalCircle GetEncompassingSphericalCircle(const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB);
	SphericalCircle GetEncompassingSphericalCircle(const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB, const DirectX::XMFLOAT3& pointC);
}

namespace Brawler
{
	template <typename Vertex>
		requires HasNormal<Vertex>
	SphericalCircle GetAdjustedSphericalCircleForTwoPoints(const std::span<const Vertex> processedVertexSpan, const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB)
	{
		SphericalCircle adjustedSphericalCircle = GetEncompassingSphericalCircle(pointA, pointB);

		for (const auto& vertex : processedVertexSpan)
		{
			const DirectX::XMFLOAT3& currNormal{ vertex.GetNormal() };

			if (!adjustedSphericalCircle.IsPointWithinCircle(currNormal))
				adjustedSphericalCircle = GetEncompassingSphericalCircle(pointA, pointB, currNormal);
		}

		return adjustedSphericalCircle;
	}
}

namespace Brawler
{
	template <typename Vertex>
		requires HasNormal<Vertex>
	void NormalBoundingConeSolver<Vertex>::CalculateNormalBoundingCone(const std::span<const Vertex> vertexSpan)
	{
		assert(!vertexSpan.empty());

		// Reset the current SphericalCircle instance.
		mCircle = SphericalCircle{};

		// We assume that all relevant normal vectors are already normalized. In Debug builds, we
		// assert if this is not the case.

		// Set the center point to be that of the first normal we find.
		{
			const DirectX::XMFLOAT3& firstNormal{ vertexSpan[0].GetNormal() };
			AssertVectorNormalization(firstNormal);

			mCircle.CenterPoint = firstNormal;
		}

		if (vertexSpan.size() == 1) [[unlikely]]
			return;

		{
			const DirectX::XMFLOAT3& secondNormal{ vertexSpan[1].GetNormal() };
			AssertVectorNormalization(secondNormal);

			mCircle = GetEncompassingSphericalCircle(mCenterPoint, secondNormal);
		}

		for (const auto i : std::views::iota(2u, vertexSpan.size()))
		{
			const DirectX::XMFLOAT3& currNormal{ vertexSpan[i].GetNormal() };
			AssertVectorNormalization(currNormal);

			// Check if the current normal represented as a point projected onto the unit sphere
			// lies within the current spherical circle.
			if (!mCircle.IsPointWithinCircle(currNormal))
				AdjustSphericalCircleForPoint(vertexSpan.subspan(0, i), currNormal);
		}
	}

	template <typename Vertex>
		requires HasNormal<Vertex>
	const DirectX::XMFLOAT3& NormalBoundingConeSolver<Vertex>::GetConeNormal() const
	{
		AssertVectorNormalization(mCircle.CenterPoint);
		return mCircle.CenterPoint;
	}

	template <typename Vertex>
		requires HasNormal<Vertex>
	float NormalBoundingConeSolver<Vertex>::GetNegativeSineAngle() const
	{
		// It is recommended in "Optimizing the Graphics Pipeline with Compute" that the angle
		// be expanded slightly in order to not reject valid normals during runtime.
		
		return -std::sin(mCircle.ConeAngle + EPSILON);
	}

	template <typename Vertex>
		requires HasNormal<Vertex>
	void NormalBoundingConeSolver<Vertex>::AdjustSphericalCircleForPoint(const std::span<const Vertex> processedVertexSpan, const DirectX::XMFLOAT3& point)
	{
		SphericalCircle adjustedSphericalCircle{ GetEncompassingSphericalCircle(processedVertexSpan[0].GetNormal(), point) };

		for (const auto i : std::views::iota(1u, processedVertexSpan.size()))
		{
			// We don't need to continue asserting that the vectors are normalized if we already do
			// so in NormalBoundingConeSolver::CalculateNormalBoundingCone().

			const DirectX::XMFLOAT3& currNormal{ processedVertexSpan[i].GetNormal() };

			if (!adjustedSphericalCircle.IsPointWithinCircle(currNormal))
				adjustedSphericalCircle = GetAdjustedSphericalCircleForTwoPoints(currNormal, point);
		}

		mCircle = std::move(adjustedSphericalCircle);
	}
}