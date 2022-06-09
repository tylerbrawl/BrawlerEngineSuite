module;
#include <array>
#include <vector>
#include <span>
#include <DirectXMath/DirectXMath.h>

export module Brawler.NormalBoundingCones:NormalBoundingConeTriangleGrouper;
import :NormalBoundingConeTypes;

namespace Brawler
{
	enum class NormalOrientationClassifier
	{
		ABOVE_XZ_ABOVE_YZ,
		ABOVE_XZ_BELOW_YZ,
		BELOW_XZ_ABOVE_YZ,
		BELOW_XZ_BELOW_YZ,

		COUNT_OR_ERROR
	};

	template <typename Vertex>
		requires HasPosition<Vertex>
	NormalOrientationClassifier GetTriangleClassifier(const Triangle<Vertex>& triangle);
}

namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	class TriangleBucket
	{
	private:
		static constexpr std::size_t MAX_TRIANGLES_PER_BUCKET = 256;

	public:
		TriangleBucket() = default;

		TriangleBucket(const TriangleBucket& rhs) = delete;
		TriangleBucket& operator=(const TriangleBucket& rhs) = delete;

		TriangleBucket(TriangleBucket&& rhs) noexcept = default;
		TriangleBucket& operator=(TriangleBucket&& rhs) noexcept = default;

		bool CanAddTriangles() const;
		void AddTriangle(Triangle<Vertex>&& triangle);

		std::size_t GetTriangleCount() const;

	private:
		std::array<Triangle<Vertex>, MAX_TRIANGLES_PER_BUCKET> mTriangleArr;
		std::size_t mCurrIndex;
	};
}

export namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	class NormalBoundingConeTriangleGrouper
	{
	public:
		NormalBoundingConeTriangleGrouper() = default;

		NormalBoundingConeTriangleGrouper(const NormalBoundingConeTriangleGrouper& rhs) = delete;
		NormalBoundingConeTriangleGrouper& operator=(const NormalBoundingConeTriangleGrouper& rhs) = delete;

		NormalBoundingConeTriangleGrouper(NormalBoundingConeTriangleGrouper&& rhs) noexcept = default;
		NormalBoundingConeTriangleGrouper& operator=(NormalBoundingConeTriangleGrouper&& rhs) noexcept = default;

		void PrepareTriangleBuckets(const std::span<const Vertex> vertexSpan, const std::span<const std::uint16_t> indexSpan);

	private:
		std::array<std::vector<TriangleBucket<Vertex>>, std::to_underlying(NormalOrientationClassifier::COUNT_OR_ERROR)> mTriangleBucketArr;
	};
}

// ------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	NormalOrientationClassifier GetTriangleClassifier(const Triangle<Vertex>& triangle)
	{
		static constexpr DirectX::XMFLOAT3 XZ_PLANE_NORMAL{ 0.0f, 1.0f, 0.0f };
		static constexpr DirectX::XMFLOAT3 YZ_PLANE_NORMAL{ 1.0f, 0.0f, 0.0f };

		static const DirectX::XMVECTOR loadedXZPlaneNormal{ DirectX::XMLoadFloat3(&XZ_PLANE_NORMAL) };
		static const DirectX::XMVECTOR loadedYZPlaneNormal{ DirectX::XMLoadFloat3(&YZ_PLANE_NORMAL) };

		// Make sure that the MSVC isn't doing anything weird with the static const values
		// (see https://developercommunity.visualstudio.com/t/modules:-global-consts-are-zero-ed/10057491).
		assert(!DirectX::XMVector3Equal(loadedXZPlaneNormal, DirectX::XMVectorZero()) && !DirectX::XMVectorEqual(loadedYZPlaneNormal, DirectX::XMVectorZero()));

		const DirectX::XMVECTOR triangleNormal{ DirectX::XMLoadFloat3(&(triangle.GetTriangleNormal())) };

		// Check which side of the plane the triangle's normal is pointing towards. If
		// the dot product of a vector A and a normal N is greater than 0, then A points
		// above the plane represented by N. If it is less than 0, then A points below
		// said plane. Finally, if the dot produce is equal to 0, then the vector A
		// lies on the plane and is orthogonal to N.
		//
		// For our purposes, we will classify normals lying on the plane in the same
		// group as those pointing above it.

		std::uint32_t xzPlaneComparisonResult = 0;
		DirectX::XMVectorGreaterOrEqualR(&xzPlaneComparisonResult, DirectX::XMVector3Dot(triangleNormal, loadedXZPlaneNormal));

		std::uint32_t yzPlaneComparisonResult = 0;
		DirectX::XMVectorGreaterOrEqualR(&yzPlaneComparisonResult, DirectX::XMVector3Dot(triangleNormal, loadedYZPlaneNormal));

		
		if (DirectX::XMComparisonAnyTrue(xzPlaneComparisonResult))
		{
			// Lies Above (or On) XZ-Plane
			return (DirectX::XMComparisonAnyTrue(yzPlaneComparisonResult) ? NormalOrientationClassifier::ABOVE_XZ_ABOVE_YZ : NormalOrientationClassifier::ABOVE_XZ_BELOW_YZ);
		}

		// Lies Below XZ-Plane
		return (DirectX::XMComparisonAnyTrue(yzPlaneComparisonResult) ? NormalOrientationClassifier::BELOW_XZ_ABOVE_YZ : NormalOrientationClassifier::BELOW_XZ_BELOW_YZ);
	}
}

namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	bool TriangleBucket<Vertex>::CanAddTriangles() const
	{
		return (GetTriangleCount() < mTriangleArr.size());
	}

	template <typename Vertex>
		requires HasPosition<Vertex>
	void TriangleBucket<Vertex>::AddTriangle(Triangle<Vertex>&& triangle)
	{
		assert(CanAddTriangles());
		mTriangleArr[mCurrIndex++] = std::move(triangle);
	}

	template <typename Vertex>
		requires HasPosition<Vertex>
	std::size_t TriangleBucket<Vertex>::GetTriangleCount() const
	{
		return mCurrIndex;
	}
}

namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	void NormalBoundingConeTriangleGrouper<Vertex>::PrepareTriangleBuckets(const std::span<const Vertex> vertexSpan, const std::span<const std::uint16_t> indexSpan)
	{
		assert(indexSpan.size() % 3 == 0);

		for (std::size_t i = 0; i < indexSpan.size(); i += 3)
		{
			const std::span<const std::uint16_t, 3> currIndexSpan{ indexSpan.subspan(i, 3) };
			Triangle<Vertex> currTriangle{ vertexSpan, currIndexSpan };

			const NormalOrientationClassifier orientationClassifier{ GetTriangleClassifier(currTriangle) };
			std::vector<TriangleBucket<Vertex>>& currBucketArr{ mTriangleBucketArr[std::to_underlying(orientationClassifier)] };

			if (currBucketArr.empty() || !currBucketArr.back().CanAddTriangles())
				currBucketArr.emplace_back();

			assert(currBucketArr.back().CanAddTriangles());
			currBucketArr.back().AddTriangle(std::move(currTriangle));
		}
	}
}