module;
#include <cassert>
#include <compare>
#include <DirectXMath/DirectXMath.h>

module Brawler.TransformComponent;
import Brawler.Math.MathConstants;
import Brawler.SceneNode;

namespace
{
	consteval Brawler::Math::Float4x4 GetDefaultWorldMatrix()
	{
		const Brawler::Math::Float4x4 defaultScaleMatrix{ Brawler::Math::GetIdentityMatrix4x4() };
		
		Brawler::Math::Float4x4 rotationAndTranslationMatrix{};

		{
			const Brawler::Math::Float3x3 condensedRotationMatrix{ Brawler::Math::IDENTITY_QUATERNION.ConvertToRotationMatrix() };

			rotationAndTranslationMatrix = Brawler::Math::Float4x4{
				condensedRotationMatrix.GetElement(0, 0), condensedRotationMatrix.GetElement(0, 1), condensedRotationMatrix.GetElement(0, 2), 0.0f,
				condensedRotationMatrix.GetElement(1, 0), condensedRotationMatrix.GetElement(1, 1), condensedRotationMatrix.GetElement(1, 2), 0.0f,
				condensedRotationMatrix.GetElement(2, 0), condensedRotationMatrix.GetElement(2, 1), condensedRotationMatrix.GetElement(2, 2), 0.0f,

				// The default translation value is the vector [0 0 0].
				0.0f, 0.0f, 0.0f, 1.0f
			};
		}

		return defaultScaleMatrix * rotationAndTranslationMatrix;
	}

	constexpr Brawler::Math::Float4x4 DEFAULT_WORLD_MATRIX{ GetDefaultWorldMatrix() };
}

namespace Brawler
{
	TransformComponent::TransformComponent() :
		mWorldMatrix(DEFAULT_WORLD_MATRIX),
		mScale(DirectX::XMFLOAT3{ 1.0f, 1.0f, 1.0f }),
		mRotation(Math::IDENTITY_QUATERNION),
		mTranslation(),
		mIsWorldMatrixDirty(false),
		mHasWorldMatrixChangedThisUpdate(false)
	{
		// To complete the construction of the world matrix, we need to append it to the world matrix
		// of the parent SceneNode, should it exist. This "append" operation is actually just a copy
		// of the value in this case, because by default, the world matrix of a TransformComponent is
		// just the identity matrix.
		//
		// It's possible that the TransformComponent is created and added to a SceneNode instance before
		// said instance is inserted into a SceneGraph. In that case, we explicitly mark the world matrix
		// as dirty so that when the SceneNode is added to the SceneGraph, it will get the correct parent
		// world matrix during the next SceneGraph update.
		const SceneNode& currNode{ GetSceneNode() };

		if (currNode.HasParentSceneNode()) [[likely]]
		{
			const TransformComponent* const parentNodeTransformPtr = currNode.GetParentSceneNode().GetComponent<const TransformComponent>();

			if (parentNodeTransformPtr != nullptr) [[likely]]
				mWorldMatrix = parentNodeTransformPtr->GetWorldMatrix();
		}
		else [[unlikely]]
			MarkWorldMatrixAsDirty();
	}

	void TransformComponent::Update(const float dt)
	{
		mHasWorldMatrixChangedThisUpdate = false;

		CheckParentWorldMatrix();
		
		if (mIsWorldMatrixDirty) [[unlikely]]
			ReBuildWorldMatrix();
	}

	void TransformComponent::SetXScale(const float scaleValue)
	{
		mScale.SetX(scaleValue);
		MarkWorldMatrixAsDirty();
	}

	void TransformComponent::SetYScale(const float scaleValue)
	{
		mScale.SetY(scaleValue);
		MarkWorldMatrixAsDirty();
	}

	void TransformComponent::SetZScale(const float scaleValue)
	{
		mScale.SetZ(scaleValue);
		MarkWorldMatrixAsDirty();
	}

	void TransformComponent::RotateAboutXAxis(const float rotationInRadians)
	{
		const Math::Quaternion xRotationQuaternion{ Math::X_AXIS, rotationInRadians };
		mRotation.ChainRotation(xRotationQuaternion);

		MarkWorldMatrixAsDirty();
	}

	void TransformComponent::RotateAboutYAxis(const float rotationInRadians)
	{
		const Math::Quaternion yRotationQuaternion{ Math::Y_AXIS, rotationInRadians };
		mRotation.ChainRotation(yRotationQuaternion);

		MarkWorldMatrixAsDirty();
	}

	void TransformComponent::RotateAboutZAxis(const float rotationInRadians)
	{
		const Math::Quaternion zRotationQuaternion{ Math::Z_AXIS, rotationInRadians };
		mRotation.ChainRotation(zRotationQuaternion);

		MarkWorldMatrixAsDirty();
	}

	void TransformComponent::TranslateInXDirection(const float distanceInMeters)
	{
		Translate(Math::Float3{ DirectX::XMFLOAT3{ distanceInMeters, 0.0f, 0.0f } });
	}

	void TransformComponent::TranslateInYDirection(const float distanceInMeters)
	{
		Translate(Math::Float3{ DirectX::XMFLOAT3{ 0.0f, distanceInMeters, 0.0f } });
	}

	void TransformComponent::TranslateInZDirection(const float distanceInMeters)
	{
		Translate(Math::Float3{ DirectX::XMFLOAT3{ 0.0f, 0.0f, distanceInMeters } });
	}

	void TransformComponent::Scale(const Math::Float3& scaleAdjustment)
	{
		mScale += scaleAdjustment;
		MarkWorldMatrixAsDirty();
	}

	void TransformComponent::Rotate(const Math::Quaternion& chainedRotation)
	{
		assert(chainedRotation.IsNormalized());
		mRotation.ChainRotation(chainedRotation);

		MarkWorldMatrixAsDirty();
	}

	void TransformComponent::Translate(const Math::Float3& translationAdjustment)
	{
		mTranslation += translationAdjustment;
		MarkWorldMatrixAsDirty();
	}

	void TransformComponent::SetScale(Math::Float3&& scale)
	{
		mScale = std::move(scale);
		MarkWorldMatrixAsDirty();
	}

	void TransformComponent::SetRotation(Math::Quaternion&& rotation)
	{
		mRotation = std::move(rotation);
		MarkWorldMatrixAsDirty();
	}

	void TransformComponent::SetTranslation(Math::Float3&& translation)
	{
		mTranslation = std::move(translation);
		MarkWorldMatrixAsDirty();
	}

	const Math::Float3& TransformComponent::GetScale() const
	{
		return mScale;
	}

	const Math::Quaternion& TransformComponent::GetRotation() const
	{
		return mRotation;
	}

	const Math::Float3& TransformComponent::GetTranslation() const
	{
		return mTranslation;
	}

	const Math::Float4x4& TransformComponent::GetWorldMatrix() const
	{
		return mWorldMatrix;
	}

	bool TransformComponent::HasWorldMatrixChangedThisUpdate() const
	{
		return mHasWorldMatrixChangedThisUpdate;
	}

	bool TransformComponent::IsWorldMatrixDirty() const
	{
		return mIsWorldMatrixDirty;
	}

	void TransformComponent::CheckParentWorldMatrix()
	{
		// Check if the world matrix of this SceneNode's parent has changed this update. If so,
		// then we need to re-build the world matrix. Otherwise, we assume that the "parent"
		// transform is the identity matrix.

		const SceneNode& currNode{ GetSceneNode() };

		if (!currNode.HasParentSceneNode()) [[unlikely]]
			return;

		const TransformComponent* const parentNodeTransformPtr = currNode.GetParentSceneNode().GetComponent<const TransformComponent>();

		if (parentNodeTransformPtr != nullptr && parentNodeTransformPtr->HasWorldMatrixChangedThisUpdate()) [[unlikely]]
			MarkWorldMatrixAsDirty();
	}

	void TransformComponent::ReBuildWorldMatrix()
	{
		assert(IsWorldMatrixDirty() && "ERROR: TransformComponent::ReBuildWorldMatrix() was called even though the world matrix was not considered dirty!");

		const Math::Float4x4 scaleMatrix{
			mScale.GetX(), 0.0f, 0.0f, 0.0f,
			0.0f, mScale.GetY(), 0.0f, 0.0f,
			0.0f, 0.0f, mScale.GetZ(), 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};

		// We can actually bring this down to two matrix multiplies by "packing" the translation
		// information into the same matrix as the rotation matrix.
		Math::Float4x4 rotationAndTranslationMatrix{};

		{
			const Math::Float3x3 compactRotationMatrix{ mRotation.ConvertToRotationMatrix() };

			rotationAndTranslationMatrix = Math::Float4x4{
				compactRotationMatrix.GetElement(0, 0), compactRotationMatrix.GetElement(0, 1), compactRotationMatrix.GetElement(0, 2), 0.0f,
				compactRotationMatrix.GetElement(1, 0), compactRotationMatrix.GetElement(1, 1), compactRotationMatrix.GetElement(1, 2), 0.0f,
				compactRotationMatrix.GetElement(2, 0), compactRotationMatrix.GetElement(2, 1), compactRotationMatrix.GetElement(2, 2), 0.0f,
				mTranslation.GetX(), mTranslation.GetY(), mTranslation.GetZ(), 1.0f
			};
		}

		mWorldMatrix = scaleMatrix * rotationAndTranslationMatrix;

		// Check for the parent transform and concatenate parentOffsetMatrix to it. If the
		// parent transform does not exist, then we assume that it is the identity transform.
		const SceneNode& currNode{ GetSceneNode() };

		if (currNode.HasParentSceneNode()) [[likely]]
		{
			const TransformComponent* const parentNodeTransformPtr = currNode.GetParentSceneNode().GetComponent<const TransformComponent>();
			
			if (parentNodeTransformPtr != nullptr) [[likely]]
			{
				assert(!parentNodeTransformPtr->IsWorldMatrixDirty() && "ERROR: A parent SceneNode's world matrix was still marked as dirty when a child node attempted to access it!");
				mWorldMatrix = (parentNodeTransformPtr->GetWorldMatrix() * mWorldMatrix);
			}
		}

		mIsWorldMatrixDirty = false;
		mHasWorldMatrixChangedThisUpdate = true;
	}

	void TransformComponent::MarkWorldMatrixAsDirty()
	{
		mIsWorldMatrixDirty = true;
	}
}