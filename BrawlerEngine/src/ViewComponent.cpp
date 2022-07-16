module;
#include <cstdint>
#include <cmath>
#include <DirectXMath/DirectXMath.h>

module Brawler.ViewComponent;
import Brawler.Math.MathConstants;
import Util.Math;
import Brawler.SceneNode;
import Brawler.TransformComponent;

// A lot of the equations here were taken from "Introduction to 3D Game Programming with DirectX 12" by
// Frank Luna. Yes, this is supposed to be an introductory text, but the realm of computer graphics is
// so vast that I find it hard to believe that experts in the field can simply recall/derive all of this
// by memory when there is so much else which they need to research. So, don't feel ashamed if you have
// to look back at these texts every now and then; I know that I don't!

namespace
{
	// ==================================================================================================================
	// View Matrix Defaults
	// ==================================================================================================================

	// There's no "right" answer for the starting view direction. In my opinion, however, having the
	// view look down the +Z-axis makes the most sense.
	constexpr Brawler::Math::Float3 DEFAULT_VIEW_DIRECTION{ Brawler::Math::Z_AXIS };
	static_assert(DEFAULT_VIEW_DIRECTION.IsNormalized(), "ERROR: The default view direction vector should be normalized!");

	constexpr Brawler::Math::Float3 DEFAULT_WORLD_UP_DIRECTION{ Brawler::Math::Y_AXIS };
	static_assert(DEFAULT_WORLD_UP_DIRECTION.IsNormalized(), "ERROR: The default world up direction vector should be normalized!");
	
	consteval Brawler::Math::Quaternion GetDefaultViewSpaceQuaternion()
	{
		const Brawler::Math::Float3 cameraPositiveZAxis{ DEFAULT_VIEW_DIRECTION };
		const Brawler::Math::Float3 cameraPositiveXAxis{ DEFAULT_WORLD_UP_DIRECTION.Cross(cameraPositiveZAxis) };
		const Brawler::Math::Float3 cameraPositiveYAxis{ cameraPositiveZAxis.Cross(cameraPositiveXAxis) };

		const Brawler::Math::Float3x3 cameraSpaceOrthonormalBasis{
			cameraPositiveXAxis.GetX(), cameraPositiveXAxis.GetY(), cameraPositiveXAxis.GetZ(),
			cameraPositiveYAxis.GetX(), cameraPositiveYAxis.GetY(), cameraPositiveYAxis.GetZ(),
			cameraPositiveZAxis.GetX(), cameraPositiveZAxis.GetY(), cameraPositiveZAxis.GetZ()
		};

		// This describes the view space coordinate system relative to world space, and can thus be used
		// to transform view space vectors into world space vectors. (Note the explicit use of the term
		// vectors here: Since quaternions have no position, they cannot store/represent the origin of
		// the coordinate system, so points cannot be transformed using a quaternion alone.) However, we need 
		// the opposite transformation, so we need to take the inverse of this matrix.
		//
		// Thankfully, since this is an orthonormal basis, it is represented by an orthogonal matrix.
		// For any orthogonal matrix M, Inverse(M) == Transpose(M).

		return Brawler::Math::Quaternion{ cameraSpaceOrthonormalBasis.Transpose() };
	}

	static constexpr Brawler::Math::Quaternion DEFAULT_VIEW_SPACE_QUATERNION{ GetDefaultViewSpaceQuaternion() };

	// ==================================================================================================================
	// Projection Matrix Defaults
	// ==================================================================================================================
	static constexpr bool DEFAULT_USE_REVERSE_Z_VALUE = true;

	// NOTE: Even if reverse Z-depth is being used, the near and far plane values shall still represent
	// the distances to the near and far planes, respectively. The only thing different with reverse
	// Z-depth is that the Z-range [n, f] in view space maps to the Z-range [1, 0] in NDC space.

	static constexpr float DEFAULT_NEAR_PLANE_DISTANCE_METERS = 0.005f;  // n = 0.5 cm
	static constexpr float DEFAULT_FAR_PLANE_DISTANCE_METERS = 5000.0f;  // f = 5 km

	static_assert(DEFAULT_NEAR_PLANE_DISTANCE_METERS <= DEFAULT_FAR_PLANE_DISTANCE_METERS, "ERROR: The near plane cannot be \"in front of\" the far plane!");

	constexpr float ConvertHorizontalFOVToVerticalFOV(const float horizontalFOVInRadians, const float aspectRatio)
	{
		// NOTE: If you know anything about my transcendental function implementations, you'll
		// know that they are ridiculously slow. However, at runtime, they take the "fast-ish"
		// path of using the equivalent <cmath> functions.
		
		const float tanHorizontalFOVOverTwo = Util::Math::GetTangentAngle(horizontalFOVInRadians / 2.0f);
		return (2.0f * Util::Math::GetArcTangentValue(tanHorizontalFOVOverTwo / aspectRatio));
	}

	static constexpr Brawler::Math::UInt2 DEFAULT_VIEW_DIMENSIONS{ DirectX::XMUINT2{ 512, 512 } };
	static constexpr float DEFAULT_VERTICAL_FOV_IN_RADIANS = Brawler::Math::HALF_PI;  // The equivalent constexpr expression is Brawler::Math::DegreesToRadians(90.0f).

	struct ProjectionMatrixParameters
	{
		Brawler::Math::UInt2 ViewDimensions;
		float VerticalFOVInRadians;
		float NearPlaneDistance;
		float FarPlaneDistance;
	};

	template <bool UseReverseZDepth>
	constexpr Brawler::Math::Float4x4 CreateProjectionMatrix(const ProjectionMatrixParameters& params)
	{
		const float aspectRatio = (static_cast<float>(params.ViewDimensions.GetX()) / static_cast<float>(params.ViewDimensions.GetY()));
		const float tangentHalfVerticalFOV = Util::Math::GetTangentAngle(params.VerticalFOVInRadians / 2.0f);

		assert(params.FarPlaneDistance >= params.NearPlaneDistance);
		const float farMinusNearDistance = (params.FarPlaneDistance - params.NearPlaneDistance);

		// Build the projection matrix differently depending on whether or not this view uses reverse
		// Z-depth. Specifically, if "traditional" Z-depth is used, then the view-space Z-range [n, f]
		// maps to the NDC-space Z-range [0, 1]. Otherwise, if reverse Z-depth is used, then the
		// view-space Z-range [n, f] maps to the NDC-space Z-range [1, 0].
		//
		// In both cases, however, n actually *DOES* refer to the distance to the near plane, and f
		// refers to the distance to the far plane. The only difference is in how the mapping to NDC
		// coordinates is done.
		if constexpr (DEFAULT_USE_REVERSE_Z_VALUE)
		{
			return Brawler::Math::Float4x4{
				(1.0f / (aspectRatio * tangentHalfVerticalFOV)), 0.0f, 0.0f, 0.0f,
				0.0f, (1.0f / tangentHalfVerticalFOV), 0.0f, 0.0f,
				0.0f, 0.0f, (-params.NearPlaneDistance / farMinusNearDistance), 1.0f,
				0.0f, 0.0f, (params.NearPlaneDistance * params.FarPlaneDistance / farMinusNearDistance), 0.0f
			};
		}
		else
		{
			return Brawler::Math::Float4x4{
				(1.0f / (aspectRatio * tangentHalfVerticalFOV)), 0.0f, 0.0f, 0.0f,
				0.0f, (1.0f / tangentHalfVerticalFOV), 0.0f, 0.0f,
				0.0f, 0.0f, (params.FarPlaneDistance / farMinusNearDistance), 1.0f,
				0.0f, 0.0f, (-(params.NearPlaneDistance * params.FarPlaneDistance) / farMinusNearDistance), 0.0f
			};
		}
	}

	static constexpr Brawler::Math::Float4x4 DEFAULT_PROJECTION_MATRIX{ []() 
	{
		const ProjectionMatrixParameters defaultParams{
			.ViewDimensions{ DEFAULT_VIEW_DIMENSIONS },
			.VerticalFOVInRadians = DEFAULT_VERTICAL_FOV_IN_RADIANS,
			.NearPlaneDistance = DEFAULT_NEAR_PLANE_DISTANCE_METERS,
			.FarPlaneDistance = DEFAULT_FAR_PLANE_DISTANCE_METERS
		};

		return CreateProjectionMatrix<DEFAULT_USE_REVERSE_Z_VALUE>(defaultParams);
	}() };
}

namespace Brawler
{
	// Since all of our math types have constexpr functions, we can actually always create
	// ViewComponent instances at runtime with "clean" view space quaternions and projection 
	// matrices! In addition, if we ever change any of the values above, then the calculated 
	// value will correctly change with it!
	//
	// Unfortunately, however, we still need to do the view-projection matrix calculation
	// within the constructor, since the complete view matrix needs the position of the view
	// in world space.

	ViewComponent::ViewComponent() :
		mViewSpaceQuaternion(DEFAULT_VIEW_SPACE_QUATERNION),
		mViewDirection(DEFAULT_VIEW_DIRECTION),
		mIsViewSpaceQuaternionDirty(false),
		mProjectionMatrix(DEFAULT_PROJECTION_MATRIX),
		mVerticalFOVRadians(DEFAULT_VERTICAL_FOV_IN_RADIANS),
		mViewDimensions(DEFAULT_VIEW_DIMENSIONS),
		mNearPlaneDistance(DEFAULT_NEAR_PLANE_DISTANCE_METERS),
		mFarPlaneDistance(DEFAULT_FAR_PLANE_DISTANCE_METERS),
		mIsProjectionMatrixDirty(false),
		mViewProjectionMatrix(),
		mUseReverseZDepth(DEFAULT_USE_REVERSE_Z_VALUE)
	{
		ReBuildViewProjectionMatrix();
	}

	void ViewComponent::Update(const float dt)
	{
		const bool viewProjectionMatrixNeedsUpdate = (mIsViewSpaceQuaternionDirty || mIsProjectionMatrixDirty);

		if (mIsViewSpaceQuaternionDirty) [[unlikely]]
			ReBuildViewSpaceQuaternion();

		if (mIsProjectionMatrixDirty) [[unlikely]]
			ReBuildProjectionMatrix();

		if (viewProjectionMatrixNeedsUpdate) [[unlikely]]
			ReBuildViewProjectionMatrix();
	}

	void ViewComponent::ReBuildViewSpaceQuaternion()
	{
		assert(mIsViewSpaceQuaternionDirty && "ERROR: ViewComponent::ReBuildViewSpaceQuaternion() was called even though the associated quaternion was never marked as dirty!");
		assert(mViewDirection.IsNormalized() && "ERROR: The view direction was never normalized prior to the update of the view space quaternion of a ViewComponent!");

		// For now, we always use the +Y axis as the "starting" up-direction for the calculation
		// of the view space orthonormal basis.
		const Math::Float3 cameraSpacePositiveXAxis{ DEFAULT_WORLD_UP_DIRECTION.Cross(mViewDirection) };
		const Math::Float3 cameraSpacePositiveYAxis{ mViewDirection.Cross(cameraSpacePositiveXAxis) };

		assert(cameraSpacePositiveXAxis.IsNormalized() && cameraSpacePositiveYAxis.IsNormalized());

		// This describes the view space coordinate system relative to world space, and can thus be used
		// to transform view space vectors into world space vectors. (Note the explicit use of the term
		// vectors here: Since quaternions have no position, they cannot store/represent the origin of
		// the coordinate system, so points cannot be transformed using a quaternion alone.) However, we need 
		// the opposite transformation, so we need to take the inverse of this matrix.
		//
		// Thankfully, since this is an orthonormal basis, it is represented by an orthogonal matrix.
		// For any orthogonal matrix M, Inverse(M) == Transpose(M).

		const Math::Float3x3 inverseViewSpaceOrthonormalBasis{
			cameraSpacePositiveXAxis.GetX(), cameraSpacePositiveYAxis.GetX(), mViewDirection.GetX(),
			cameraSpacePositiveXAxis.GetY(), cameraSpacePositiveYAxis.GetY(), mViewDirection.GetY(),
			cameraSpacePositiveXAxis.GetZ(), cameraSpacePositiveYAxis.GetZ(), mViewDirection.GetZ()
		};
		
		mViewSpaceQuaternion = Math::Quaternion{ inverseViewSpaceOrthonormalBasis };
		mIsViewSpaceQuaternionDirty = false;
	}

	void ViewComponent::MarkViewSpaceQuaternionAsDirty()
	{
		mIsViewSpaceQuaternionDirty = true;
	}

	void ViewComponent::ReBuildProjectionMatrix()
	{
		assert(mIsProjectionMatrixDirty && "ERROR: ViewComponent::ReBuildProjectionMatrix() was called even though the associated matrix was never marked as dirty!");

		const ProjectionMatrixParameters matrixParams{
			.ViewDimensions{mViewDimensions},
			.VerticalFOVInRadians = mVerticalFOVRadians,
			.NearPlaneDistance = mNearPlaneDistance,
			.FarPlaneDistance = mFarPlaneDistance
		};

		if (mUseReverseZDepth) [[likely]]
			mProjectionMatrix = CreateProjectionMatrix<true>(matrixParams);
		else [[unlikely]]
			mProjectionMatrix = CreateProjectionMatrix<false>(matrixParams);

		mIsProjectionMatrixDirty = false;
	}

	void ViewComponent::MarkProjectionMatrixAsDirty()
	{
		mIsProjectionMatrixDirty = true;
	}

	void ViewComponent::ReBuildViewProjectionMatrix()
	{
		// Get the world space position of the SceneNode; we'll need it to define the origin of the
		// view space coordinate system.
		const TransformComponent* const sceneNodeTransformPtr{ GetSceneNode().GetComponent<TransformComponent>() };
		assert(sceneNodeTransformPtr != nullptr && "ERROR: A SceneNode was given a ViewComponent, but not a TransformComponent! (The ViewComponent relies on the TransformComponent to define the origin of the view space coordinate system.)");

		const Math::Float3 negatedSceneNodeTranslation{ sceneNodeTransformPtr->GetTranslation() * -1.0f };

		// Construct the complete view matrix by combining the orthogonal matrix constructed from
		// the view space quaternion with the translation.
		Math::Float4x4 viewSpaceMatrix{ mViewSpaceQuaternion.ConvertToRotationMatrix().ExpandDimensions() };
	}
}