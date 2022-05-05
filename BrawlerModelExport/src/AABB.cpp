module;
#include <utility>
#include <DirectXMath/DirectXMath.h>

module Brawler.Math.AABB;

namespace Brawler
{
	namespace Math
	{
		AABB::AABB(DirectX::XMFLOAT3&& minPoint, DirectX::XMFLOAT3&& maxPoint) :
			mMinPoint(std::move(minPoint)),
			mMaxPoint(std::move(maxPoint))
		{}

		void XM_CALLCONV AABB::InsertPoint(const DirectX::FXMVECTOR point)
		{
			DirectX::XMVECTOR minPoint{ DirectX::XMLoadFloat3(&mMinPoint) };
			minPoint = DirectX::XMVectorMin(minPoint, point);
			DirectX::XMStoreFloat3(&mMinPoint, minPoint);

			DirectX::XMVECTOR maxPoint{ DirectX::XMLoadFloat3(&mMaxPoint) };
			maxPoint = DirectX::XMVectorMax(maxPoint, point);
			DirectX::XMStoreFloat3(&mMaxPoint, maxPoint);
		}

		void AABB::InsertAABB(const AABB& boundingBox)
		{
			const DirectX::XMVECTOR minPoint{ DirectX::XMVectorMin(DirectX::XMLoadFloat3(&mMinPoint), DirectX::XMLoadFloat3(&(boundingBox.mMinPoint))) };
			DirectX::XMStoreFloat3(&mMinPoint, minPoint);

			const DirectX::XMVECTOR maxPoint{ DirectX::XMVectorMax(DirectX::XMLoadFloat3(&mMaxPoint), DirectX::XMLoadFloat3(&(boundingBox.mMaxPoint))) };
			DirectX::XMStoreFloat3(&mMaxPoint, maxPoint);
		}

		const DirectX::XMFLOAT3& AABB::GetMinimumBoundingPoint() const
		{
			return mMinPoint;
		}

		const DirectX::XMFLOAT3& AABB::GetMaximumBoundingPoint() const
		{
			return mMaxPoint;
		}
	}
}