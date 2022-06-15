module;
#include <span>
#include <array>
#include <vector>
#include <cassert>
#include <DirectXMath/DirectXMath.h>

export module Brawler.NormalBoundingCones:NormalBoundingConeTypes;
import Util.General;

export namespace Brawler
{
	struct NormalBoundingCone
	{
		DirectX::XMFLOAT3 ConeNormal;
		float NegativeSineAngle;
	};

	template <typename Vertex>
	concept HasPosition = requires (const Vertex & v)
	{
		{ v.GetPosition() } -> std::same_as<const DirectX::XMFLOAT3&>;
	};
}

export namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	class Triangle
	{
	public:
		Triangle() = default;
		Triangle(const std::span<const Vertex> vertexSpan, std::array<std::uint16_t, 3>&& indexArr);

		Triangle(const Triangle& rhs) = default;
		Triangle& operator=(const Triangle& rhs) = default;

		Triangle(Triangle&& rhs) noexcept = default;
		Triangle& operator=(Triangle&& rhs) noexcept = default;

		std::span<const std::uint16_t, 3> GetIndexSpan() const;
		const DirectX::XMFLOAT3& GetTriangleNormal() const;

	private:
		std::array<std::uint16_t, 3> mIndexArr;
		DirectX::XMFLOAT3 mTriangleNormal;
	};
}

export namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	struct NormalBoundingConeTriangleGroup
	{
		NormalBoundingCone BoundingCone;
		std::vector<Triangle<Vertex>> TriangleArr;
	};
}

// --------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename Vertex>
		requires HasPosition<Vertex>
	Triangle<Vertex>::Triangle(const std::span<const Vertex> vertexSpan, std::array<std::uint16_t, 3>&& indexArr) :
		mIndexArr(std::move(indexArr)),
		mTriangleNormal()
	{
		if constexpr (Util::General::IsDebugModeEnabled())
		{
			for (const auto index : mIndexArr)
				assert(index < vertexSpan.size() && "ERROR: An out-of-bounds index was detected when re-constructing triangles for normal bounding cone bucketization!");
		}

		const DirectX::XMFLOAT3& positionA{ vertexSpan[mIndexArr[0]].GetPosition() };
		const DirectX::XMFLOAT3& positionB{ vertexSpan[mIndexArr[1]].GetPosition() };
		const DirectX::XMFLOAT3& positionC{ vertexSpan[mIndexArr[2]].GetPosition() };

		const DirectX::XMVECTOR vectorAB{ DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&positionB), DirectX::XMLoadFloat3(&positionA)) };
		const DirectX::XMVECTOR vectorBC{ DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&positionC), DirectX::XMLoadFloat3(&positionB)) };

		const DirectX::XMVECTOR triangleNormal{ DirectX::XMVector3Normalize(DirectX::XMVector3Cross(vectorAB, vectorBC)) };
		DirectX::XMStoreFloat3(&mTriangleNormal, triangleNormal);
	}

	template <typename Vertex>
		requires HasPosition<Vertex>
	std::span<const std::uint16_t, 3> Triangle<Vertex>::GetIndexSpan() const
	{
		return std::span<const std::uint16_t, 3>{ mIndexArr };
	}

	template <typename Vertex>
		requires HasPosition<Vertex>
	const DirectX::XMFLOAT3& Triangle<Vertex>::GetTriangleNormal() const
	{
		return mTriangleNormal;
	}
}