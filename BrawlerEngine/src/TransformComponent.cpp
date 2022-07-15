module;
#include <cassert>
#include <compare>
#include <DirectXMath/DirectXMath.h>

module Brawler.TransformComponent;
import Brawler.Math.MathConstants;

namespace Brawler
{
	TransformComponent::TransformComponent() :
		mWorldMatrix(),
		mScale(DirectX::XMFLOAT3{ 1.0f, 1.0f, 1.0f }),
		mRotation(Math::IDENTITY_QUATERNION),
		mTranslation(),
		mIsWorldMatrixDirty(true)
	{
		ReBuildWorldMatrix();
	}

	void TransformComponent::Update(const float dt)
	{
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
		assert(!IsWorldMatrixDirty() && "ERROR: TransformComponent::GetWorldMatrix() *MUST* be a thread-safe function so that child nodes in the SceneGraph can concurrently get its value! Thus, it cannot be called if TransformComponent::IsWorldMatrixDirty() returns true!");
		return mWorldMatrix;
	}

	bool TransformComponent::IsWorldMatrixDirty() const
	{
		return mIsWorldMatrixDirty;
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
		mIsWorldMatrixDirty = false;
	}

	void TransformComponent::MarkWorldMatrixAsDirty()
	{
		mIsWorldMatrixDirty = true;
	}
}