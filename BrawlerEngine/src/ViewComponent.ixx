module;
#include <cstdint>

export module Brawler.ViewComponent;
import Brawler.I_Component;
import Brawler.Math.MathTypes;

export namespace Brawler
{
	class ViewComponent final : public I_Component
	{
	public:
		ViewComponent();

		ViewComponent(const ViewComponent& rhs) = delete;
		ViewComponent& operator=(const ViewComponent& rhs) = delete;

		ViewComponent(ViewComponent&& rhs) noexcept = default;
		ViewComponent& operator=(ViewComponent&& rhs) noexcept = default;

		void Update(const float dt);

		void SetViewDirection(Math::Float3&& normalizedViewDirection);

		void SetVerticalFieldOfView(const float fovInRadians);
		void SetHorizontalFieldOfView(const float fovInRadians);

		void SetViewDimensions(const std::uint32_t width, const std::uint32_t height);

		void SetNearPlaneDistance(const float nearPlaneDistanceInMeters);
		void SetFarPlaneDistance(const float farPlaneDistanceInMeters);

		void SetReverseZDepthState(const bool useReverseZ);

	private:
		void ReBuildViewSpaceQuaternion();
		void MarkViewSpaceQuaternionAsDirty();

		void ReBuildProjectionMatrix();
		void MarkProjectionMatrixAsDirty();

		void ReBuildViewProjectionMatrix();

	private:
		/// <summary>
		/// A quaternion can compactly store a rotation. As it turns out, every rotation can be
		/// represented as an orthogonal matrix, and every orthogonal matrix represents a unique
		/// rotation.
		/// 
		/// What this means in practice is that we can store any orthonormal basis as a quaternion!
		/// </summary>
		Math::Quaternion mViewSpaceQuaternion;
		Math::Float3 mViewDirection;
		bool mIsViewSpaceQuaternionDirty;

		Math::Float4x4 mProjectionMatrix;
		float mVerticalFOVRadians;
		Math::UInt2 mViewDimensions;
		float mNearPlaneDistance;
		float mFarPlaneDistance;
		bool mIsProjectionMatrixDirty;

		Math::Float4x4 mViewProjectionMatrix;

		bool mUseReverseZDepth;
	};
}