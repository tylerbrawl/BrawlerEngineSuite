module;
#include <DirectXMath/DirectXMath.h>

export module Brawler.Math.AABB;

export namespace Brawler
{
	namespace Math
	{
		class AABB
		{
		public:
			/// <summary>
			/// Creates an axis-aligned bounding box (AABB) with the specified minimum and
			/// maximum bounding points.
			/// </summary>
			/// <param name="minPoint">
			/// - The minimum bounding point of the AABB.
			/// </param>
			/// <param name="maxPoint">
			/// - The maximum bounding point of the AABB.
			/// </param>
			AABB(DirectX::XMFLOAT3&& minPoint, DirectX::XMFLOAT3&& maxPoint);

			/// <summary>
			/// Adjusts the boundaries of the axis-aligned bounding box (AABB) so that it
			/// includes the position specified by point.
			/// </summary>
			/// <param name="point">
			/// - The point which is to be inserted into the AABB. If necessary, the boundaries
			///   of the AABB will be adjusted to include it.
			/// </param>
			void XM_CALLCONV InsertPoint(const DirectX::FXMVECTOR point);

			/// <summary>
			/// Adjusts the boundaries of the axis-aligned bounding box (AABB) so that it
			/// includes the entire AABB specified by boundingBox.
			/// 
			/// In a more mathematical sense, this AABB is unioned with boundingBox to produce
			/// the new value of this AABB.
			/// </summary>
			/// <param name="boundingBox">
			/// - The axis-aligned bounding box which is to be inserted into the AABB. If necessary, 
			///   the boundaries of this AABB will be adjusted to include it.
			/// </param>
			void InsertAABB(const AABB& boundingBox);

			/// <summary>
			/// Retrieves the minimum bounding point of the axis-aligned bounding box (AABB).
			/// </summary>
			/// <returns>
			/// The function returns the minimum bounding point of the AABB.
			/// </returns>
			const DirectX::XMFLOAT3& GetMinimumBoundingPoint() const;

			/// <summary>
			/// Retrieves the maximum bounding point of the axis-aligned bounding box (AABB).
			/// </summary>
			/// <returns>
			/// The function returns the maximum bounding point of the AABB.
			/// </returns>
			const DirectX::XMFLOAT3& GetMaximumBoundingPoint() const;

		private:
			DirectX::XMFLOAT3 mMinPoint;
			DirectX::XMFLOAT3 mMaxPoint;
		};
	}
}