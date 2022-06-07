module;
#include <cassert>
#include <DirectXMath/DirectXMath.h>

module Brawler.NormalBoundingConeSolver;
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
			.ConeAngle{ DirectX::XMVectorGetX(DirectX::XMVector3AngleBetweenNormals(centerPoint, pointA)) }
		};
	}
	
	SphericalCircle GetEncompassingSphericalCircle(const DirectX::XMFLOAT3& pointA, const DirectX::XMFLOAT3& pointB, const DirectX::XMFLOAT3& pointC)
	{
		// What follows is a bunch of mathematical madness. There might be an easier way to do this, but
		// as it stands, we're already bottlenecked by texture compression, so it probably isn't worth investigating.

		const DirectX::XMVECTOR loadedA{ DirectX::XMLoadFloat3(&pointA) };
		const DirectX::XMVECTOR loadedB{ DirectX::XMLoadFloat3(&pointB) };
		const DirectX::XMVECTOR loadedC{ DirectX::XMLoadFloat3(&pointC) };

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
		// the equation would leave v unconstrained. However, we can amend that by instead using the following
		// equation:
		//
		// P1.x + (t * V1.x) - (2 * (P2.x + (u * V2.x))) = -(P3.x + (v * V3.x))
		//
		// In essence, we are saying that if A = B, then A - 2B = -A. To constrain v, then, we can change -A
		// to an equation -C containing v, where A = C. This simplifies to the following:
		//
		// (V1.x)t - 2(V2.x)u + (V3.x)v = 2(P2.x) - P1.x - P3.x
		//
		// This in turn can be written into the augmented matrix used to solve the system as follows:
		//
		// [V1.x    -2(V2.x)    V3.x  |  2(P2.x) - P1.x - P3.x]
		//
		// The process is analogous for the Y- and Z-components.
		//
		// DirectXMath does not offer much of an ability to perform row reduction operations on actual matrices,
		// so we need to create a "matrix" using multiple DirectX::XMVECTOR instances.

		DirectX::XMVECTOR systemRow1{};
		DirectX::XMVECTOR systemRow2{};
		DirectX::XMVECTOR systemRow3{};

		{
			DirectX::XMFLOAT3 storedMidpointABToCenterPointVector{};
			DirectX::XMStoreFloat3(&storedMidpointABToCenterPointVector, midpointABToCenterPointVector);

			DirectX::XMFLOAT3 storedScaledMidpointBCToCenterPointVector{};
			DirectX::XMStoreFloat3(&storedScaledMidpointBCToCenterPointVector, DirectX::XMVectorScale(midpointBCToCenterPointVector, -2.0f));

			DirectX::XMFLOAT3 storedMidpointCAToCenterPointVector{};
			DirectX::XMStoreFloat3(&storedMidpointCAToCenterPointVector, midpointCAToCenterPointVector);

			DirectX::XMFLOAT3 storedSystemSolutionsVector{};
			DirectX::XMStoreFloat3(&storedSystemSolutionsVector, DirectX::XMVectorSubtract(DirectX::XMVectorSubtract(DirectX::XMVectorScale(midpointBC, 2.0f), midpointAB), midpointCA));

			systemRow1 = DirectX::XMVectorSet(
				storedMidpointABToCenterPointVector.x,
				storedScaledMidpointBCToCenterPointVector.x,
				storedMidpointCAToCenterPointVector.x,
				storedSystemSolutionsVector.x
			);

			systemRow2 = DirectX::XMVectorSet(
				storedMidpointABToCenterPointVector.y,
				storedScaledMidpointBCToCenterPointVector.y,
				storedMidpointCAToCenterPointVector.y,
				storedSystemSolutionsVector.y
			);

			systemRow3 = DirectX::XMVectorSet(
				storedMidpointABToCenterPointVector.z,
				storedScaledMidpointBCToCenterPointVector.z,
				storedMidpointCAToCenterPointVector.z,
				storedSystemSolutionsVector.z
			);
		}

		// Thankfully, we don't actually need to solve for all of t, u, and v. Since we know that the final
		// result will be the same regardless of which one we choose, we can just fully reduce one row and
		// use the corresponding equation to solve for the center point. In this case, we will be solving
		// for v.

		systemRow2 = DirectX::XMVectorSubtract(systemRow2, DirectX::XMVectorScale(systemRow1, -(DirectX::XMVectorGetX(systemRow2) / DirectX::XMVectorGetX(systemRow1))));
		systemRow3 = DirectX::XMVectorSubtract(systemRow3, DirectX::XMVectorScale(systemRow1, -(DirectX::XMVectorGetX(systemRow3) / DirectX::XMVectorGetX(systemRow1))));

		systemRow3 = DirectX::XMVectorSubtract(systemRow3, DirectX::XMVectorScale(systemRow2, -(DirectX::XMVectorGetY(systemRow3) / DirectX::XMVectorGetY(systemRow2))));

		systemRow3 = DirectX::XMVectorScale(systemRow3, 1.0f / DirectX::XMVectorGetZ(systemRow3));

		const float v = DirectX::XMVectorGetW(systemRow3);

		// We now have the multiple used to scale midpointCAToCenterPointVector in L3 in order to get
		// the center point.
		const DirectX::XMVECTOR centerPoint{ DirectX::XMVector3Normalize(DirectX::XMVectorAdd(midpointCA, DirectX::XMVectorScale(midpointCAToCenterPointVector, v))) };

		DirectX::XMFLOAT3 storedCenterPoint{};
		DirectX::XMStoreFloat3(&storedCenterPoint, centerPoint);

		return SphericalCircle{
			.CenterPoint{ std::move(storedCenterPoint) },
			.ConeAngle{ DirectX::XMVectorGetX(DirectX::XMVector3AngleBetweenNormals(centerPoint, pointA)) }
		};
	}
}