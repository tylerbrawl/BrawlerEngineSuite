module;
#include <compare>

export module Brawler.TransformComponent;
import Brawler.I_Component;
import Brawler.Math.MathTypes;

export namespace Brawler
{
	class TransformComponent final : public I_Component
	{
	public:
		TransformComponent();

		TransformComponent(const TransformComponent& rhs) = delete;
		TransformComponent& operator=(const TransformComponent& rhs) = delete;

		TransformComponent(TransformComponent&& rhs) noexcept = default;
		TransformComponent& operator=(TransformComponent&& rhs) noexcept = default;

		void Update(const float dt) override;

		void SetXScale(const float scaleValue);
		void SetYScale(const float scaleValue);
		void SetZScale(const float scaleValue);

		void RotateAboutXAxis(const float rotationInRadians);
		void RotateAboutYAxis(const float rotationInRadians);
		void RotateAboutZAxis(const float rotationInRadians);

		void TranslateInXDirection(const float distanceInMeters);
		void TranslateInYDirection(const float distanceInMeters);
		void TranslateInZDirection(const float distanceInMeters);

		void Scale(const Math::Float3& scaleAdjustment);
		void Rotate(const Math::Quaternion& chainedRotation);
		void Translate(const Math::Float3& translationAdjustment);

		void SetScale(Math::Float3&& scale);
		void SetRotation(Math::Quaternion&& rotation);
		void SetTranslation(Math::Float3&& translation);

		const Math::Float3& GetScale() const;
		const Math::Quaternion& GetRotation() const;
		const Math::Float3& GetTranslation() const;

		const Math::Float4x4& GetWorldMatrix() const;
		bool IsWorldMatrixDirty() const;

	private:
		void ReBuildWorldMatrix();
		void MarkWorldMatrixAsDirty();

	private:
		Math::Float4x4 mWorldMatrix;
		Math::Float3 mScale;
		Math::Quaternion mRotation;
		Math::Float3 mTranslation;
		bool mIsWorldMatrixDirty;
	};
}