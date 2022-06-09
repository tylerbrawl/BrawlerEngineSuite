module;
#include <cassert>
#include <utility>
#include <cmath>
#include <DirectXMath/DirectXMath.h>

module Brawler.NormalBoundingCones;
import Util.General;

namespace Brawler
{
	void AssertVectorNormalization(const DirectX::XMFLOAT3& vector)
	{
		if constexpr (Util::General::IsDebugModeEnabled())
		{
			const DirectX::XMVECTOR loadedVector{ DirectX::XMLoadFloat3(&vector) };
			const DirectX::XMVECTOR normalizedVector{ DirectX::XMVector3Normalize(loadedVector) };

			assert(DirectX::XMVector3NearEqual(loadedVector, normalizedVector, DirectX::XMVectorReplicate(EPSILON)) && "ERROR: An unnormalized normal vector was detected when solving for a normal bounding cone!");
		}
	}

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

	SphericalCircle GetEncompassingSphericalCircle(const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB)
	{
		// Getting the spherical circle which passes through two points is much easier than getting the one which
		// passes through three. We just need to find the midpoint of the two points.

		const DirectX::XMVECTOR loadedA{ DirectX::XMLoadFloat3(&pointA) };
		const DirectX::XMVECTOR loadedB{ DirectX::XMLoadFloat3(&pointB) };

		const DirectX::XMVECTOR centerPoint{ DirectX::XMVector3Normalize(DirectX::XMVectorScale(DirectX::XMVectorAdd(loadedA, loadedB), 0.5f)) };

		DirectX::XMFLOAT3 storedCenterPoint{};
		DirectX::XMStoreFloat3(&storedCenterPoint, centerPoint);

		return SphericalCircle{
			.CenterPoint{std::move(storedCenterPoint)},
			.ConeAngle{ DirectX::XMVectorGetX(DirectX::XMVector3AngleBetweenNormals(centerPoint, loadedA)) }
		};
	}

	DirectX::XMFLOAT2 GetEncompassing2DCircleCenterPoint(const DirectX::XMFLOAT2& pointA2D, const DirectX::XMFLOAT2& pointB2D, const DirectX::XMFLOAT2& pointC2D)
	{
		// For the sake of reasoning, let's pretend that pointA2D, pointB2D, and pointC2D all lie on the XY-plane
		// (i.e., they have the same Z-values). GetEncompassingSphericalCircle() knows which plane the points
		// really lie on, but for solving this problem, that is irrelevant.

		// No, this cannot be made constexpr.
		const DirectX::XMVECTOR planeNormal{ DirectX::XMVectorSet(
			0.0f,
			0.0f,
			1.0f,
			0.0f
		) };

		const DirectX::XMVECTOR loadedA{ DirectX::XMLoadFloat2(&pointA2D) };
		const DirectX::XMVECTOR loadedB{ DirectX::XMLoadFloat2(&pointB2D) };
		const DirectX::XMVECTOR loadedC{ DirectX::XMLoadFloat2(&pointC2D) };

		const DirectX::XMVECTOR midpointAB{ DirectX::XMVectorScale(DirectX::XMVectorAdd(loadedA, loadedB), 0.5f) };
		const DirectX::XMVECTOR midpointBC{ DirectX::XMVectorScale(DirectX::XMVectorAdd(loadedB, loadedC), 0.5f) };

		const DirectX::XMVECTOR vectorAB{ DirectX::XMVectorSubtract(loadedB, loadedA) };
		const DirectX::XMVECTOR vectorBC{ DirectX::XMVectorSubtract(loadedC, loadedB) };

		const DirectX::XMVECTOR midpointABToCenterPoint{ DirectX::XMVector3Cross(planeNormal, vectorAB) };
		const DirectX::XMVECTOR midpointBCToCenterPoint{ DirectX::XMVector3Cross(planeNormal, vectorBC) };

		// Similar to the situation in three dimensions, we have a system of equations to solve. This time,
		// however, we only need to worry about two equations. Let
		// L1 = midpointAB + t * (midpointABToCenterPoint) and
		// L2 = midpointBC + u * (midpointBCToCenterPoint)
		// be two lines which lie on the Z-plane. We want to find the point on the Z-plane at which the lines
		// L1 and L2 intersect.
		
		DirectX::XMVECTOR systemRow1{};
		DirectX::XMVECTOR systemRow2{};

		{
			DirectX::XMFLOAT2 storedMidpointAB{};
			DirectX::XMStoreFloat2(&storedMidpointAB, midpointAB);

			DirectX::XMFLOAT2 storedMidpointBC{};
			DirectX::XMStoreFloat2(&storedMidpointBC, midpointBC);

			DirectX::XMFLOAT2 storedMidpointABToCenterPoint{};
			DirectX::XMStoreFloat2(&storedMidpointABToCenterPoint, midpointABToCenterPoint);

			DirectX::XMFLOAT2 storedNegatedMidpointBCToCenterPoint{};
			DirectX::XMStoreFloat2(&storedNegatedMidpointBCToCenterPoint, DirectX::XMVectorNegate(midpointBCToCenterPoint));

			DirectX::XMFLOAT2 storedSystemSolutions{};
			DirectX::XMStoreFloat2(&storedSystemSolutions, DirectX::XMVectorSubtract(midpointBC, midpointAB));

			systemRow1 = DirectX::XMVectorSet(
				storedMidpointABToCenterPoint.x,
				storedNegatedMidpointBCToCenterPoint.x,
				0.0f,
				storedSystemSolutions.x
			);

			systemRow2 = DirectX::XMVectorSet(
				storedMidpointABToCenterPoint.y,
				storedNegatedMidpointBCToCenterPoint.y,
				0.0f,
				storedSystemSolutions.y
			);
		}

		if (std::abs(DirectX::XMVectorGetX(systemRow1)) < std::abs(DirectX::XMVectorGetX(systemRow2)))
			std::swap(systemRow1, systemRow2);

		static constexpr float SCALE_VALUE_COMPARISON_EPSILON = 0.000001f;

		{
			const float row1ScaleValue = DirectX::XMVectorGetX(systemRow1);

			if (std::abs(row1ScaleValue) > SCALE_VALUE_COMPARISON_EPSILON) [[likely]]
				systemRow2 = DirectX::XMVectorSubtract(systemRow2, DirectX::XMVectorScale(systemRow1, (DirectX::XMVectorGetX(systemRow2) / row1ScaleValue)));
		}

		{
			const float row2ScaleValue = DirectX::XMVectorGetY(systemRow2);
			systemRow2 = DirectX::XMVectorScale(systemRow2, 1.0f / row2ScaleValue);
		}

		const float u = DirectX::XMVectorGetW(systemRow2);

		DirectX::XMFLOAT2 centerPoint{};
		DirectX::XMStoreFloat2(&centerPoint, DirectX::XMVectorAdd(midpointBC, DirectX::XMVectorScale(midpointBCToCenterPoint, u)));

		assert(!std::isnan(centerPoint.x) && !std::isnan(centerPoint.y));

		return centerPoint;
	}
	
	SphericalCircle GetEncompassingSphericalCircle(const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB, const DirectX::XMFLOAT3& pointC)
	{
		static constexpr float SWITCH_TO_DIFFERENT_MODE_EPSILON = 0.005f;
		
		// What follows is a bunch of mathematical madness. There might be an easier way to do this, but
		// as it stands, we're already bottlenecked by texture compression, so it probably isn't worth investigating.

		const DirectX::XMVECTOR loadedA{ DirectX::XMLoadFloat3(&pointA) };
		const DirectX::XMVECTOR loadedB{ DirectX::XMLoadFloat3(&pointB) };
		const DirectX::XMVECTOR loadedC{ DirectX::XMLoadFloat3(&pointC) };

		{
			const DirectX::XMVECTOR epsilonVector{ DirectX::XMVectorReplicate(SWITCH_TO_DIFFERENT_MODE_EPSILON) };

			if (DirectX::XMVector3NearEqual(loadedA, loadedB, epsilonVector) && DirectX::XMVector3NearEqual(loadedA, loadedC, epsilonVector)) [[unlikely]]
				return SphericalCircle{
					.CenterPoint{pointA},
					.ConeAngle = 0.0f
			};

			if (DirectX::XMVector3NearEqual(loadedA, loadedB, epsilonVector) || DirectX::XMVector3NearEqual(loadedB, loadedC, epsilonVector)) [[unlikely]]
				return GetEncompassingSphericalCircle(pointA, pointC);

			if (DirectX::XMVector3NearEqual(loadedA, loadedC, epsilonVector)) [[unlikely]]
				return GetEncompassingSphericalCircle(pointA, pointB);
		}

		/*
		if (std::abs(pointA.x - pointB.x) <= SWITCH_TO_DIFFERENT_MODE_EPSILON && std::abs(pointA.x - pointC.x) <= SWITCH_TO_DIFFERENT_MODE_EPSILON) [[unlikely]]
		{
			const DirectX::XMFLOAT2 pointA2D{ pointA.y, pointA.z };
			const DirectX::XMFLOAT2 pointB2D{ pointB.y, pointB.z };
			const DirectX::XMFLOAT2 pointC2D{ pointC.y, pointC.z };

			const DirectX::XMFLOAT2 centerPoint2D{ GetEncompassing2DCircleCenterPoint(pointA2D, pointB2D, pointC2D) };

			DirectX::XMVECTOR centerPoint3D{ DirectX::XMVectorSet(
				pointA.x,
				centerPoint2D.x,
				centerPoint2D.y,
				0.0f
			) };
			centerPoint3D = DirectX::XMVector3Normalize(centerPoint3D);

			SphericalCircle circle{};
			DirectX::XMStoreFloat3(&(circle.CenterPoint), centerPoint3D);

			circle.ConeAngle = DirectX::XMVectorGetX(DirectX::XMVector3AngleBetweenNormals(centerPoint3D, loadedA));

			return circle;
		}

		if (std::abs(pointA.y - pointB.y) <= SWITCH_TO_DIFFERENT_MODE_EPSILON && std::abs(pointA.y - pointC.y) <= SWITCH_TO_DIFFERENT_MODE_EPSILON) [[unlikely]]
		{
			const DirectX::XMFLOAT2 pointA2D{ pointA.x, pointA.z };
			const DirectX::XMFLOAT2 pointB2D{ pointB.x, pointB.z };
			const DirectX::XMFLOAT2 pointC2D{ pointC.x, pointC.z };

			const DirectX::XMFLOAT2 centerPoint2D{ GetEncompassing2DCircleCenterPoint(pointA2D, pointB2D, pointC2D) };

			DirectX::XMVECTOR centerPoint3D{ DirectX::XMVectorSet(
				centerPoint2D.x,
				pointA.y,
				centerPoint2D.y,
				0.0f
			) };
			centerPoint3D = DirectX::XMVector3Normalize(centerPoint3D);

			SphericalCircle circle{};
			DirectX::XMStoreFloat3(&(circle.CenterPoint), centerPoint3D);

			circle.ConeAngle = DirectX::XMVectorGetX(DirectX::XMVector3AngleBetweenNormals(centerPoint3D, loadedA));

			return circle;
		}

		if (std::abs(pointA.z - pointB.z) <= SWITCH_TO_DIFFERENT_MODE_EPSILON && std::abs(pointA.z - pointC.z) <= SWITCH_TO_DIFFERENT_MODE_EPSILON) [[unlikely]]
		{
			const DirectX::XMFLOAT2 pointA2D{ pointA.x, pointA.y };
			const DirectX::XMFLOAT2 pointB2D{ pointB.x, pointB.y };
			const DirectX::XMFLOAT2 pointC2D{ pointC.x, pointC.y };

			const DirectX::XMFLOAT2 centerPoint2D{ GetEncompassing2DCircleCenterPoint(pointA2D, pointB2D, pointC2D) };

			DirectX::XMVECTOR centerPoint3D{ DirectX::XMVectorSet(
				centerPoint2D.x,
				centerPoint2D.y,
				pointA.z,
				0.0f
			) };
			centerPoint3D = DirectX::XMVector3Normalize(centerPoint3D);

			SphericalCircle circle{};
			DirectX::XMStoreFloat3(&(circle.CenterPoint), centerPoint3D);

			circle.ConeAngle = DirectX::XMVectorGetX(DirectX::XMVector3AngleBetweenNormals(centerPoint3D, loadedA));

			return circle;
		}
		*/

		// Get the plane containing the points A, B, and C. Note that DirectX::XMPlaneFromPoints(A, B, C)
		// uses the cross product BA x CA to determine the normal.
		const DirectX::XMVECTOR planeEquation{ DirectX::XMPlaneFromPoints(loadedA, loadedB, loadedC) };

		// Find the midpoints between A and B and between B and C.
		const DirectX::XMVECTOR midpointAB{ DirectX::XMVectorScale(DirectX::XMVectorAdd(loadedA, loadedB), 0.5f) };
		const DirectX::XMVECTOR midpointBC{ DirectX::XMVectorScale(DirectX::XMVectorAdd(loadedB, loadedC), 0.5f) };
		const DirectX::XMVECTOR midpointCA{ DirectX::XMVectorScale(DirectX::XMVectorAdd(loadedC, loadedA), 0.5f) };

		// We want to find the vectors orthogonal to the vectors AB and BC. The problem is that since we are
		// working in three dimensions, there are an infinite number of these vectors. The ones we want also
		// lie in the plane containing A, B, and C.
		//
		// We know that the final center point is going to be equidistant from points A, B, and C.

		const DirectX::XMVECTOR vectorAB{ DirectX::XMVectorSubtract(loadedB, loadedA) };
		const DirectX::XMVECTOR midpointABToCenterPointVector{ DirectX::XMVector3Cross(planeEquation, vectorAB) };

		const DirectX::XMVECTOR vectorBC{ DirectX::XMVectorSubtract(loadedC, loadedB) };
		const DirectX::XMVECTOR midpointBCToCenterPointVector{ DirectX::XMVector3Cross(planeEquation, vectorBC) };

		const DirectX::XMVECTOR vectorCA{ DirectX::XMVectorSubtract(loadedA, loadedC) };
		const DirectX::XMVECTOR midpointCAToCenterPointVector{ DirectX::XMVector3Cross(planeEquation, vectorCA) };

		// Let L1 = midpointAB + t * (midpointABToCenterPointVector),
		// L2 = midpointBC + u * (midpointBCToCenterPointVector), and
		// L3 = midpointCA + v * (midpointCAToCenterPointVector) be two lines which lie on the plane containing
		// points pointA, pointB, and pointC. The point at which L1, L2, and L3 intersect is the center of the
		// circle which passes through pointA, pointB, and pointC.
		//
		// This is a system of equations, so we need to use Gaussian elimination/row reduction to solve it.
		// To demonstrate how the equations are generated, we will take a look at the X-component of all
		// relevant vectors. Our goal is to find the values of t, u, and v such that all three line equations
		// produce the same point.
		//
		// Normally, when you have two lines, you would have something like P1.x + (t * V1.x) = P2.x + (u * V2.x),
		// where PN and VN are the origin and direction vector of line LN, respectively. In this case, however,
		// the equation would leave v unconstrained. As it turns out, however, we can define v in terms of the
		// other variables with the following equation:
		//
		// P1.x + (t * V1.x) - (2 * (P2.x + (u * V2.x))) = -(P3.x + (v * V3.x))
		//
		// In essence, we are saying that if A = B, then A - 2B = -A. To constrain v, then, we can change -A
		// to an equation -C containing v, where A = C. This simplifies to the following:
		//
		// (V1.x)t - 2(V2.x)u + (V3.x)v = 2(P2.x) - P1.x - P3.x
		//
		// Since we can define v in terms of the other variables, we know that is is *NOT* linearly independent
		// of the other equations. Thus, we only need to worry about t and u. In fact, we can just re-use the
		// equation we listed above for the case with two lines, writing the row of the augmented matrix
		// as follows:
		//
		// [(V1.x)    -(V2.x)  |  P2.x - P1.x]

		DirectX::XMVECTOR systemRow1{};
		DirectX::XMVECTOR systemRow2{};

		{
			DirectX::XMFLOAT3 storedMidpointABToCenterPointVector{};
			DirectX::XMStoreFloat3(&storedMidpointABToCenterPointVector, midpointABToCenterPointVector);

			DirectX::XMFLOAT3 storedNegatedMidpointBCToCenterPointVector{};
			DirectX::XMStoreFloat3(&storedNegatedMidpointBCToCenterPointVector, DirectX::XMVectorNegate(midpointBCToCenterPointVector));

			DirectX::XMFLOAT3 storedSystemSolutionsVector{};
			DirectX::XMStoreFloat3(&storedSystemSolutionsVector, DirectX::XMVectorSubtract(midpointBC, midpointAB));

			systemRow1 = DirectX::XMVectorSet(
				storedMidpointABToCenterPointVector.x,
				storedNegatedMidpointBCToCenterPointVector.x,
				0.0f,
				storedSystemSolutionsVector.x
			);

			systemRow2 = DirectX::XMVectorSet(
				storedMidpointABToCenterPointVector.y,
				storedNegatedMidpointBCToCenterPointVector.y,
				0.0f,
				storedSystemSolutionsVector.y
			);
		}

		// We actually don't need to solve for both t and u. Since we know that the final result will be the 
		// same regardless of which one we choose, we can just fully reduce one row and use the corresponding 
		// equation to solve for the center point. In this case, we will be solving for u.

		if (std::abs(DirectX::XMVectorGetX(systemRow1)) < std::abs(DirectX::XMVectorGetX(systemRow2)))
			std::swap(systemRow1, systemRow2);

		static constexpr float SCALE_VALUE_COMPARISON_EPSILON = 0.000001f;

		{
			const float row1ScaleValue = DirectX::XMVectorGetX(systemRow1);

			if (std::abs(row1ScaleValue) > SCALE_VALUE_COMPARISON_EPSILON) [[likely]]
				systemRow2 = DirectX::XMVectorSubtract(systemRow2, DirectX::XMVectorScale(systemRow1, (DirectX::XMVectorGetX(systemRow2) / row1ScaleValue)));
		}

		SphericalCircle circle{};

		const float row2ScaleValue = DirectX::XMVectorGetY(systemRow2);

		if (std::abs(row2ScaleValue) > SCALE_VALUE_COMPARISON_EPSILON) [[likely]]
		{
			systemRow2 = DirectX::XMVectorScale(systemRow2, 1.0f / row2ScaleValue);

			const float u = DirectX::XMVectorGetW(systemRow2);

			// We now have the multiple used to scale midpointCAToCenterPointVector in L3 in order to get
			// the center point.
			const DirectX::XMVECTOR centerPoint{ DirectX::XMVector3Normalize(DirectX::XMVectorAdd(midpointBC, DirectX::XMVectorScale(midpointBCToCenterPointVector, u))) };
			DirectX::XMStoreFloat3(&(circle.CenterPoint), centerPoint);

			circle.ConeAngle = DirectX::XMVectorGetX(DirectX::XMVector3AngleBetweenNormals(centerPoint, loadedA));
		}
		
		else
		{
			// If that failed, then there are infinitely many solutions to the problem.
			DirectX::XMVECTOR centerPoint{};

			float row1ScaleValue = DirectX::XMVectorGetX(systemRow1);

			if (std::abs(row1ScaleValue) > SCALE_VALUE_COMPARISON_EPSILON) [[likely]]
			{
				systemRow1 = DirectX::XMVectorScale(systemRow1, 1.0f / row1ScaleValue);

				const float t = DirectX::XMVectorGetW(systemRow1);
				centerPoint = DirectX::XMVector3Normalize(DirectX::XMVectorAdd(midpointAB, DirectX::XMVectorScale(midpointABToCenterPointVector, t)));
			}

			else
			{
				row1ScaleValue = DirectX::XMVectorGetY(systemRow1);
				assert(std::abs(row1ScaleValue) > SCALE_VALUE_COMPARISON_EPSILON);

				systemRow1 = DirectX::XMVectorScale(systemRow1, 1.0f / row1ScaleValue);

				const float u = DirectX::XMVectorGetW(systemRow1);
				centerPoint = DirectX::XMVector3Normalize(DirectX::XMVectorAdd(midpointBC, DirectX::XMVectorScale(midpointBCToCenterPointVector, u)));
			}
			
			DirectX::XMStoreFloat3(&(circle.CenterPoint), centerPoint);
			circle.ConeAngle = DirectX::XMVectorGetX(DirectX::XMVector3AngleBetweenNormals(centerPoint, loadedA));
		}
		
		assert(!std::isnan(circle.CenterPoint.x) && !std::isnan(circle.CenterPoint.y) && !std::isnan(circle.CenterPoint.z));

		return circle;
	}
}