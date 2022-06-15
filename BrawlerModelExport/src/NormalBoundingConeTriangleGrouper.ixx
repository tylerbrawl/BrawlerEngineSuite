module;
#include <array>
#include <vector>
#include <span>
#include <DirectXMath/DirectXMath.h>

export module Brawler.NormalBoundingCones:NormalBoundingConeTriangleGrouper;
import :NormalBoundingConeTypes;
import :NormalBoundingConeSolver;
import Brawler.JobSystem;

namespace Brawler
{
	enum class NormalOrientationClassifier
	{
		ABOVE_XY_ABOVE_XZ_ABOVE_YZ,
		ABOVE_XY_ABOVE_XZ_BELOW_YZ,
		ABOVE_XY_BELOW_XZ_ABOVE_YZ,
		ABOVE_XY_BELOW_XZ_BELOW_YZ,
		BELOW_XY_ABOVE_XZ_ABOVE_YZ,
		BELOW_XY_ABOVE_XZ_BELOW_YZ,
		BELOW_XY_BELOW_XZ_ABOVE_YZ,
		BELOW_XY_BELOW_XZ_BELOW_YZ,

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

		void SolveNormalBoundingCone(const std::span<const Vertex> vertexSpan);
		NormalBoundingConeTriangleGroup<Vertex> GetBoundingConeTriangleGroup();

	private:
		std::array<Triangle<Vertex>, MAX_TRIANGLES_PER_BUCKET> mTriangleArr;
		NormalBoundingCone mBoundingCone;
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
		void SolveNormalBoundingCones();

		std::vector<NormalBoundingConeTriangleGroup<Vertex>> GetNormalBoundingConeTriangleGroups();

	private:
		std::span<const Vertex> mVertexSpan;
		std::array<std::vector<TriangleBucket<Vertex>>, std::to_underlying(NormalOrientationClassifier::COUNT_OR_ERROR)> mTriangleBucketArr;
		std::size_t mTriangleBucketCount;
	};
}

// ------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	NormalOrientationClassifier GetTriangleClassifier(const Triangle<Vertex>& triangle)
	{
		static constexpr DirectX::XMFLOAT3 XY_PLANE_NORMAL{ 0.0f, 0.0f, 1.0f };
		static constexpr DirectX::XMFLOAT3 XZ_PLANE_NORMAL{ 0.0f, 1.0f, 0.0f };
		static constexpr DirectX::XMFLOAT3 YZ_PLANE_NORMAL{ 1.0f, 0.0f, 0.0f };

		static const DirectX::XMVECTOR loadedXYPlaneNormal{ DirectX::XMLoadFloat3(&XY_PLANE_NORMAL) };
		static const DirectX::XMVECTOR loadedXZPlaneNormal{ DirectX::XMLoadFloat3(&XZ_PLANE_NORMAL) };
		static const DirectX::XMVECTOR loadedYZPlaneNormal{ DirectX::XMLoadFloat3(&YZ_PLANE_NORMAL) };

		// Make sure that the MSVC isn't doing anything weird with the static const values
		// (see https://developercommunity.visualstudio.com/t/modules:-global-consts-are-zero-ed/10057491).
		assert(!DirectX::XMVector3Equal(loadedXYPlaneNormal, DirectX::XMVectorZero()) && !DirectX::XMVector3Equal(loadedXZPlaneNormal, DirectX::XMVectorZero()) && 
			!DirectX::XMVector3Equal(loadedYZPlaneNormal, DirectX::XMVectorZero()));

		const DirectX::XMVECTOR triangleNormal{ DirectX::XMLoadFloat3(&(triangle.GetTriangleNormal())) };

		// Check which side of the plane the triangle's normal is pointing towards. If
		// the dot product of a vector A and a normal N is greater than 0, then A points
		// above the plane represented by N. If it is less than 0, then A points below
		// said plane. Finally, if the dot product is equal to 0, then the vector A
		// lies on the plane and is orthogonal to N.
		//
		// For our purposes, we will classify normals lying on the plane in the same
		// group as those pointing above it.

		std::uint32_t xyPlaneComparisonResult = 0;
		DirectX::XMVectorGreaterOrEqualR(&xyPlaneComparisonResult, DirectX::XMVector3Dot(triangleNormal, loadedXYPlaneNormal), DirectX::XMVectorZero());

		std::uint32_t xzPlaneComparisonResult = 0;
		DirectX::XMVectorGreaterOrEqualR(&xzPlaneComparisonResult, DirectX::XMVector3Dot(triangleNormal, loadedXZPlaneNormal), DirectX::XMVectorZero());

		std::uint32_t yzPlaneComparisonResult = 0;
		DirectX::XMVectorGreaterOrEqualR(&yzPlaneComparisonResult, DirectX::XMVector3Dot(triangleNormal, loadedYZPlaneNormal), DirectX::XMVectorZero());

		NormalOrientationClassifier currClassifier = NormalOrientationClassifier::BELOW_XY_BELOW_XZ_BELOW_YZ;

		if (DirectX::XMComparisonAnyTrue(xyPlaneComparisonResult))
		{
			// Lies Above (or On) XY-Plane
			currClassifier = static_cast<NormalOrientationClassifier>(std::to_underlying(currClassifier) - 4);
		}

		if (DirectX::XMComparisonAnyTrue(xzPlaneComparisonResult))
		{
			// Lies Above (or On) XZ-Plane
			currClassifier = static_cast<NormalOrientationClassifier>(std::to_underlying(currClassifier) - 2);
		}

		if (DirectX::XMComparisonAnyTrue(yzPlaneComparisonResult))
		{
			// Lies Above (or On) YZ-Plane
			currClassifier = static_cast<NormalOrientationClassifier>(std::to_underlying(currClassifier) - 1);
		}
		
		return currClassifier;
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

	template <typename Vertex>
		requires HasPosition<Vertex>
	void TriangleBucket<Vertex>::SolveNormalBoundingCone(const std::span<const Vertex> vertexSpan)
	{
		NormalBoundingConeSolver<Vertex> boundingConeSolver{};
		const std::span<const Triangle<Vertex>> triangleSpan{ mTriangleArr.data(), GetTriangleCount() };

		mBoundingCone = boundingConeSolver.CalculateNormalBoundingCone(triangleSpan);
	}

	template <typename Vertex>
		requires HasPosition<Vertex>
	NormalBoundingConeTriangleGroup<Vertex> TriangleBucket<Vertex>::GetBoundingConeTriangleGroup()
	{
		NormalBoundingConeTriangleGroup<Vertex> triangleGroup{
			.BoundingCone{ mBoundingCone },
			.TriangleArr{}
		};

		triangleGroup.TriangleArr.reserve(GetTriangleCount());

		const std::span<Triangle<Vertex>> triangleSpan{ mTriangleArr.data(), GetTriangleCount() };

		for (auto&& triangle : triangleSpan)
			triangleGroup.TriangleArr.push_back(std::move(triangle));

		return triangleGroup;
	}
}

namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	void NormalBoundingConeTriangleGrouper<Vertex>::PrepareTriangleBuckets(const std::span<const Vertex> vertexSpan, const std::span<const std::uint16_t> indexSpan)
	{
		assert(indexSpan.size() % 3 == 0);

		mVertexSpan = vertexSpan;

		for (std::size_t i = 0; i < indexSpan.size(); i += 3)
		{
			std::array<std::uint16_t, 3> indexArr{ indexSpan[i], indexSpan[i + 1], indexSpan[i + 2] };
			Triangle<Vertex> currTriangle{ mVertexSpan, std::move(indexArr) };

			const NormalOrientationClassifier orientationClassifier{ GetTriangleClassifier(currTriangle) };
			std::vector<TriangleBucket<Vertex>>& currBucketArr{ mTriangleBucketArr[std::to_underlying(orientationClassifier)] };

			if (currBucketArr.empty() || !currBucketArr.back().CanAddTriangles())
			{
				currBucketArr.emplace_back();
				++mTriangleBucketCount;
			}

			assert(currBucketArr.back().CanAddTriangles());
			currBucketArr.back().AddTriangle(std::move(currTriangle));
		}
	}

	template <typename Vertex>
		requires HasPosition<Vertex>
	void NormalBoundingConeTriangleGrouper<Vertex>::SolveNormalBoundingCones()
	{
		assert(!mVertexSpan.empty());

		Brawler::JobGroup solveBoundingConesGroup{};
		solveBoundingConesGroup.Reserve(mTriangleBucketCount);

		for (auto& bucketArr : mTriangleBucketArr)
		{
			for (auto& bucket : bucketArr)
			{
				solveBoundingConesGroup.AddJob([&bucket, vertexSpan = mVertexSpan] ()
				{
					bucket.SolveNormalBoundingCone(vertexSpan);
				});
			}
		}

		solveBoundingConesGroup.ExecuteJobs();
	}

	template <typename Vertex>
		requires HasPosition<Vertex>
	std::vector<NormalBoundingConeTriangleGroup<Vertex>> NormalBoundingConeTriangleGrouper<Vertex>::GetNormalBoundingConeTriangleGroups()
	{
		std::vector<NormalBoundingConeTriangleGroup<Vertex>> triangleGroupArr{};
		triangleGroupArr.reserve(mTriangleBucketCount);

		for (auto& bucketArr : mTriangleBucketArr)
		{
			for (auto& bucket : bucketArr)
				triangleGroupArr.push_back(bucket.GetBoundingConeTriangleGroup());
		}

		return triangleGroupArr;
	}
}